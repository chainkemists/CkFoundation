import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import subprocess
import os
import threading

class GitLFSManager:
    def __init__(self, root):
        self.root = root
        self.root.title("Git LFS Manager")

        # Repository root folder
        self.repo_frame = tk.Frame(root)
        self.repo_frame.pack(fill='x', padx=10, pady=5)

        self.repo_root_label = tk.Label(self.repo_frame, text="Repository Root Folder:")
        self.repo_root_label.pack(side='left', padx=5)

        self.repo_root_entry = tk.Entry(self.repo_frame, width=50)
        self.repo_root_entry.pack(side='left', fill='x', expand=True, padx=5)

        self.repo_root_button = tk.Button(self.repo_frame, text="Browse...", command=self.browse_folder)
        self.repo_root_button.pack(side='left', padx=5)

        # Search bar and filter
        self.search_frame = tk.Frame(root)
        self.search_frame.pack(fill='x', padx=10, pady=5)

        self.search_label = tk.Label(self.search_frame, text="Search:")
        self.search_label.pack(side='left', padx=5)

        self.search_entry = tk.Entry(self.search_frame, width=50)
        self.search_entry.pack(side='left', fill='x', expand=True, padx=5)
        self.search_entry.bind('<KeyRelease>', self.update_filter)

        # Treeview for displaying locked files
        self.tree_frame = tk.Frame(root)
        self.tree_frame.pack(fill='both', expand=True, padx=10, pady=10)

        # Sorting state
        self.sort_column = 'User'
        self.sort_reverse = True

        self._detached = set()
        self.locked_tree = ttk.Treeview(self.tree_frame, columns=('User', 'File Name', 'File Path'), show='headings')
        self.locked_tree.heading('User', text='User', command=lambda: self.sort_by_column(self.locked_tree, 'User'))
        self.locked_tree.heading('File Name', text='File Name', command=lambda: self.sort_by_column(self.locked_tree, 'File Name'))
        self.locked_tree.heading('File Path', text='File Path', command=lambda: self.sort_by_column(self.locked_tree, 'File Path'))
        self.locked_tree.column('User', anchor='center', width=150)
        self.locked_tree.column('File Name', anchor='w', width=300)
        self.locked_tree.column('File Path', anchor='w', width=500)
        self.locked_tree.pack(side='left', fill='both', expand=True)

        # Scrollbars for the treeview
        self.locked_scrollbar = ttk.Scrollbar(self.tree_frame, orient='vertical', command=self.locked_tree.yview)
        self.locked_tree.configure(yscroll=self.locked_scrollbar.set)
        self.locked_scrollbar.pack(side='right', fill='y')

        # Button frame
        self.button_frame = tk.Frame(root)
        self.button_frame.pack(pady=10)

        # List locks button
        self.list_locks_button = tk.Button(self.button_frame, text="List Git LFS Files and Locks", command=self.async_list_lfs_files_and_locks)
        self.list_locks_button.pack(side='left', padx=10)

        # Unlock files button
        self.unlock_files_button = tk.Button(self.button_frame, text="Unlock Selected Files", command=self.unlock_files)
        self.unlock_files_button.pack(side='left', padx=10)

        # Force unlock files button
        self.force_unlock_button = tk.Button(self.button_frame, text="Force Unlock Selected Files", command=self.force_unlock_files, fg='red')
        self.force_unlock_button.pack(side='left', padx=10)

        # Initialize data storage
        self.locks_data = []

        # User color mapping
        self.user_colors = {}
        self.color_palette = ['#FFCCCC', '#CCFFCC', '#CCCCFF', '#FFCC99', '#99CCFF', '#FF99CC', '#99FFCC', '#CC99FF']
        self.color_index = 0

        # Auto-detect and set the repository root folder
        self.detect_and_set_repo_root()

    def browse_folder(self):
        folder_selected = filedialog.askdirectory()
        if folder_selected:
            self.repo_root_entry.delete(0, tk.END)
            self.repo_root_entry.insert(0, folder_selected)
            self.async_list_lfs_files_and_locks()

    def detect_and_set_repo_root(self):
        current_dir = os.getcwd()
        while True:
            if os.path.isdir(os.path.join(current_dir, '.git')):
                self.repo_root_entry.delete(0, tk.END)
                self.repo_root_entry.insert(0, current_dir)
                return
            parent_dir = os.path.dirname(current_dir)
            if parent_dir == current_dir:
                break
            current_dir = parent_dir

    def run_git_command(self, command, cwd):
        try:
            result = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
            if result.returncode == 0:
                return result.stdout
            else:
                # messagebox.showerror("Error", f"Command failed: {result.stderr}")
                return None
        except Exception as e:
            messagebox.showerror("Error", str(e))
            return None

    def update_filter(self, event=None):
        for item in self._detached:
            if self.locked_tree.exists(item):
                self.locked_tree.reattach(item, '', 'end')

        search_term = self.search_entry.get().lower()
        for item in self.locked_tree.get_children():
            values = self.locked_tree.item(item, 'values')
            user, file_name, file_path = values
            if (search_term in user.lower() or
                search_term in file_name.lower() or
                search_term in file_path.lower()):
                self.locked_tree.reattach(item, '', 'end')
            else:
                self.locked_tree.detach(item)
                self._detached.add(item)

        self.sort_by_column(self.locked_tree, "User")

    def get_column_index(self, column_name):
        column_mapping = {
            'User': 0,
            'File Name': 1,
            'File Path': 2
        }
        return column_mapping.get(column_name, 0)

    def sort_by_column(self, treeview, column):
        items = [(treeview.item(item, 'values'), item) for item in treeview.get_children()]
        column_index = self.get_column_index(column)

        if self.sort_column == column:
            self.sort_reverse = not self.sort_reverse
        else:
            self.sort_reverse = False
        self.sort_column = column

        items.sort(key=lambda x: x[0][column_index], reverse=self.sort_reverse)

        for index, (values, item) in enumerate(items):
            treeview.move(item, '', index)

        treeview.heading(column, command=lambda: self.sort_by_column(treeview, column))

    def get_column_index(self, column_name):
        column_mapping = {
            'User': 0,
            'File Name': 1,
            'File Path': 2
        }
        return column_mapping.get(column_name, 0)

    def async_list_lfs_files_and_locks(self):
        self.disable_buttons()
        threading.Thread(target=self.list_lfs_files_and_locks).start()

    def list_lfs_files_and_locks(self):
        _detached = set()
        #output_files = self.run_git_command(['git', 'lfs', 'ls-files', '--all'], cwd=self.repo_root_entry.get())
        output_locks = self.run_git_command(['git', 'lfs', 'locks'], cwd=self.repo_root_entry.get())
        locks = output_locks.strip().split('\n')[0:]  # Skip header line

        rows_to_insert = []
        if output_locks:
            for lock in locks:
                parts = lock.split()
                file_path = parts[0]
                user = parts[1]

                self.locks_data.append((user, file_path, file_path))
                if user and user not in self.user_colors:
                    self.user_colors[user] = self.color_palette[self.color_index % len(self.color_palette)]
                    self.color_index += 1

                file_name = os.path.basename(file_path)
                dir_name = os.path.dirname(file_path)
                rows_to_insert.append((user, file_name, dir_name))

        self.root.after(0, self.update_treeview, rows_to_insert)
        self.root.after(0, self.enable_buttons)
        self.update_filter()
        return

    def update_treeview(self, rows):
        self.locked_tree.delete(*self.locked_tree.get_children())
        for row in rows:
            user, file_name, file_path = row
            self.locked_tree.insert('', 'end', values=row, tags=(user,))
            self.locked_tree.tag_configure(user, background=self.user_colors.get(user, "#FFFFFF"))
        self.sort_by_column(self.locked_tree, 'User')

    def unlock_files(self):
        selected_items = self.locked_tree.selection()
        if not selected_items:
            messagebox.showinfo("Info", "No files selected to unlock.")
            return

        unlocked_files = []
        failed_files = []

        for item in selected_items:
            file_info = self.locked_tree.item(item, 'values')
            file_path = os.path.join(file_info[2], file_info[1])
            output = self.run_git_command(['git', 'lfs', 'unlock', file_path], cwd=self.repo_root_entry.get())
            if output:
                unlocked_files.append(file_path)
            else:
                failed_files.append(file_path)

        summary_message = "Unlocking operation completed:\n"
        if unlocked_files:
            summary_message += "Successfully unlocked {} file(s):\n{}\n".format(len(unlocked_files), "\n".join(unlocked_files))
        if failed_files:
            summary_message += "Failed to unlock {} file(s):\n{}".format(len(failed_files), "\n".join(failed_files))
        if not unlocked_files and not failed_files:
            summary_message = "No files selected to unlock or failed to unlock all files."

        messagebox.showinfo("Summary", summary_message)
        self.async_list_lfs_files_and_locks()

    def force_unlock_files(self):
        selected_items = self.locked_tree.selection()
        if not selected_items:
            messagebox.showinfo("Info", "No files selected to force unlock.")
            return

        force_unlocked_files = []
        failed_files = []

        for item in selected_items:
            file_info = self.locked_tree.item(item, 'values')
            file_path = os.path.join(file_info[2], file_info[1])
            output = self.run_git_command(['git', 'lfs', 'unlock', '--force', file_path], cwd=self.repo_root_entry.get())
            if output:
                force_unlocked_files.append(file_path)
            else:
                failed_files.append(file_path)

        summary_message = "Force unlocking operation completed:\n"
        if force_unlocked_files:
            summary_message += "Successfully force unlocked {} file(s):\n{}\n".format(len(force_unlocked_files), "\n".join(force_unlocked_files))
        if failed_files:
            summary_message += "Failed to force unlock {} file(s):\n{}".format(len(failed_files), "\n".join(failed_files))
        if not force_unlocked_files and not failed_files:
            summary_message = "No files selected to force unlock or failed to force unlock all files."

        messagebox.showinfo("Summary", summary_message)
        self.async_list_lfs_files_and_locks()

    def disable_buttons(self):
        self.list_locks_button.config(state='disabled')
        self.unlock_files_button.config(state='disabled')
        self.force_unlock_button.config(state='disabled')
        self.search_entry.config(state='disabled')

    def enable_buttons(self):
        self.list_locks_button.config(state='normal')
        self.unlock_files_button.config(state='normal')
        self.force_unlock_button.config(state='normal')
        self.search_entry.config(state='normal')

if __name__ == "__main__":
    root = tk.Tk()
    app = GitLFSManager(root)
    root.mainloop()
def get_column_index(self, column_name):
    column_mapping = {
        'User': 0,
        'File Name': 1,
        'File Path': 2
    }
    return column_mapping.get(column_name, 0)

def sort_by_column(self, treeview, column):
    items = [(treeview.item(item, 'values'), item) for item in treeview.get_children()]
    column_index = self.get_column_index(column)

    if self.sort_column == column:
        self.sort_reverse = not self.sort_reverse
    else:
        self.sort_reverse = False
    self.sort_column = column

    items.sort(key=lambda x: x[0][column_index], reverse=self.sort_reverse)

    for index, (values, item) in enumerate(items):
        treeview.move(item, '', index)

    treeview.heading(column, command=lambda: self.sort_by_column(treeview, column))