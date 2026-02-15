import os

CONFIG_FILE = ".config"
HEADER_FILE = "include/config.h"

defaults = {
    "CONFIG_USB_SUPPORT": "y",
    "CONFIG_SERIAL_DEBUG": "y",
    "CONFIG_MEMORY_MB": "512",
    "CONFIG_SHELL_AUTOSTART": "y"
}

def load_config():
    config = defaults.copy()
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as f:
            for line in f:
                if "=" in line:
                    key, val = line.strip().split("=")
                    config[key] = val
    return config

def save_config(config):
    with open(CONFIG_FILE, "w") as f:
        for key, val in config.items():
            f.write(f"{key}={val}\n")

    with open(HEADER_FILE, "w") as f:
        f.write("#ifndef CONFIG_H\n#define CONFIG_H\n\n")
        for key, val in config.items():
            if val == "y":
                f.write(f"#define {key} 1\n")
            elif val == "n":
                f.write(f"#undef {key}\n")
            else:
                f.write(f"#define {key} {val}\n")
        f.write("\n#endif\n")

import sys

def main():
    config = load_config()
    if "--default" in sys.argv:
        if not os.path.exists(CONFIG_FILE) or not os.path.exists(HEADER_FILE):
            save_config(config)
        return

    while True:
        print("\n--- OSx2 Configuration Menu ---")
        options = list(config.keys())
        for i, key in enumerate(options):
            print(f"{i+1}. {key}: {config[key]}")
        print("s. Save and Exit")
        print("q. Quit without saving")

        choice = input("Select an option: ").strip().lower()
        if choice == 's':
            save_config(config)
            print("Configuration saved.")
            break
        elif choice == 'q':
            break
        try:
            idx = int(choice) - 1
            if 0 <= idx < len(options):
                key = options[idx]
                if config[key] in ["y", "n"]:
                    config[key] = "n" if config[key] == "y" else "y"
                else:
                    new_val = input(f"Enter new value for {key}: ").strip()
                    if new_val:
                        config[key] = new_val
        except ValueError:
            print("Invalid choice.")

if __name__ == "__main__":
    main()
