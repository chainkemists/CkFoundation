import os
import shutil
import tkinter as tk
from tkinter import filedialog, messagebox

class TemplateReplacerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Template Replacer")

        self.frame = tk.Frame(root)
        self.frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.label1 = tk.Label(self.frame, text="Select Template Folder:")
        self.label1.pack(padx=5, pady=5, anchor=tk.W)

        self.folder_path = tk.StringVar()
        self.entry1 = tk.Entry(self.frame, textvariable=self.folder_path)
        self.entry1.pack(fill=tk.X, padx=5, pady=5)

        self.button1 = tk.Button(self.frame, text="Browse", command=self.browse_folder)
        self.button1.pack(padx=5, pady=5)

        self.label2 = tk.Label(self.frame, text="New Name:")
        self.label2.pack(padx=5, pady=5, anchor=tk.W)

        self.new_name = tk.StringVar()
        self.entry2 = tk.Entry(self.frame, textvariable=self.new_name)
        self.entry2.pack(fill=tk.X, padx=5, pady=5)

        self.label3 = tk.Label(self.frame, text="New Namespace (optional):")
        self.label3.pack(padx=5, pady=5, anchor=tk.W)

        self.new_namespace = tk.StringVar()
        self.entry3 = tk.Entry(self.frame, textvariable=self.new_namespace)
        self.entry3.pack(fill=tk.X, padx=5, pady=5)

        self.button2 = tk.Button(self.frame, text="Replace", command=self.replace_symbols)
        self.button2.pack(pady=10)

        self.log_text = tk.Text(self.frame, height=10)
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.log_text.configure(state='disabled')

        self.error_text = tk.Text(self.frame, height=5, fg='red')
        self.error_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.error_text.configure(state='disabled')

    def browse_folder(self):
        folder_selected = filedialog.askdirectory()
        self.folder_path.set(folder_selected)

    def copy_folder(self, src, dst):
        try:
            shutil.copytree(src, dst)
            return True
        except Exception as e:
            self.error_text.configure(state='normal')
            self.error_text.insert(tk.END, f"Error copying template folder: {str(e)}\n")
            self.error_text.configure(state='disabled')
            return False

    def replace_in_file(self, file_path, old_new_pairs):
        replaced_instances = []
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()

            for old, new in old_new_pairs:
                if old in content:
                    content = content.replace(old, new)
                    replaced_instances.append((old, new))

            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)

        except Exception as e:
            error_message = f"Error replacing in file {file_path}: {str(e)}\n"
            self.error_text.configure(state='normal')
            self.error_text.insert(tk.END, error_message)
            self.error_text.configure(state='disabled')

        return replaced_instances

    def replace_in_directory(self, current_dir, old_new_pairs):
        replaced_instances = []
        for subdir, dirs, files in os.walk(current_dir):
            # Replace in all files
            for file in files:
                file_path = os.path.join(subdir, file)
                replaced = self.replace_in_file(file_path, old_new_pairs)
                replaced_instances.extend(replaced)

                # Rename the file if it contains the old prefix
                new_file_name = file
                for old, new in old_new_pairs:
                    new_file_name = new_file_name.replace(old, new)
                if new_file_name != file:
                    try:
                        os.rename(file_path, os.path.join(subdir, new_file_name))
                    except Exception as e:
                        error_message = f"Error renaming file {file_path}: {str(e)}\n"
                        self.error_text.configure(state='normal')
                        self.error_text.insert(tk.END, error_message)
                        self.error_text.configure(state='disabled')

            # Rename directories
            for dir_name in dirs:
                old_dir_path = os.path.join(subdir, dir_name)
                new_dir_name = dir_name
                for old, new in old_new_pairs:
                    new_dir_name = new_dir_name.replace(old, new)
                new_dir_path = os.path.join(subdir, new_dir_name)
                if old_dir_path != new_dir_path:
                    try:
                        os.rename(old_dir_path, new_dir_path)
                        self.replace_in_directory(new_dir_path, old_new_pairs)
                    except Exception as e:
                        error_message = f"Error renaming directory {old_dir_path}: {str(e)}\n"
                        self.error_text.configure(state='normal')
                        self.error_text.insert(tk.END, error_message)
                        self.error_text.configure(state='disabled')

        return replaced_instances

    def replace_symbols(self):
        template_folder = self.folder_path.get()
        new_name = self.new_name.get()
        new_namespace = self.new_namespace.get().strip()

        if not template_folder or not new_name:
            messagebox.showerror("Error", "Please select a template folder and enter the new name.")
            return

        old_suffix = "EcsTemplate"
        old_macro_suffix = "ECSTEMPLATE"
        old_namespace = "ck::ecs_template"
        old_prefix_partial = "Ck"
        old_macro_partial = "CK"
        old_namespace_partial = "ck"

        new_prefix = f"{new_namespace}{new_name}" if new_namespace else new_name
        new_macro = f"{new_namespace.upper()}{new_name.upper()}" if new_namespace else new_name.upper()
        new_namespace_full = f'{new_namespace.lower()}::{new_name.lower()}' if new_namespace else f'ck::{new_name.lower()}'
        new_namespace_partial = new_namespace.lower() if new_namespace else "ck"

        old_new_pairs = [
            (old_suffix, new_name),
            (old_macro_suffix, new_name.upper()),
            (old_namespace, new_namespace_full),
            (old_prefix_partial + old_suffix, new_namespace + new_name),
            (old_macro_partial + old_macro_suffix, new_namespace.upper() + new_name.upper()),
            (old_namespace_partial, new_namespace_partial)
        ]

        new_template_path = os.path.join(os.path.dirname(template_folder), f"{new_prefix}_template")

        if self.copy_folder(template_folder, new_template_path):
            try:
                replaced_instances = self.replace_in_directory(new_template_path, old_new_pairs)
                self.display_log(replaced_instances)
                messagebox.showinfo("Success", "Symbols replaced successfully.")
            except Exception as e:
                error_message = f"Error replacing symbols: {str(e)}\n"
                self.error_text.configure(state='normal')
                self.error_text.insert(tk.END, error_message)
                self.error_text.configure(state='disabled')

    def display_log(self, replaced_instances):
        self.log_text.configure(state='normal')
        self.log_text.delete(1.0, tk.END)
        for old, new in replaced_instances:
            log_message = f"Replaced: {old} -> {new}\n"
            self.log_text.insert(tk.END, log_message)
        self.log_text.configure(state='disabled')

if __name__ == "__main__":
    root = tk.Tk()
    app = TemplateReplacerApp(root)
    root.mainloop()
