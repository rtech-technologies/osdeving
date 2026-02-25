#!/bin/bash
# OSx2 Configuration Setup Tool
# Looks for existing .config files and lets user choose one, or create new

clear
echo "=========================================="
echo "  OSx2 Kernel Setup"
echo "=========================================="
echo ""

# Look for existing .config files
found_configs=$(find . -maxdepth 3 -name ".config*" -not -path "./.git/*" 2>/dev/null | grep -v ".config.default")

if [ -z "$found_configs" ]; then
    echo "No existing config files found."
    echo ""
    echo "1. Create new .config (QEMU default)"
    echo "2. Create new .config (Disk image)"
    echo "3. Create new .config (ISO image)"
    echo "4. Manual edit"
    echo ""
    read -p "Choice [1-4]: " choice
    
    case $choice in
        1)
            cat > .config << 'EOF'
# OSx2 Kernel Configuration
BUILD_QEMU=1
BUILD_IMG=0
BUILD_ISO=0
EOF
            echo "✓ Created .config (QEMU)"
            ;;
        2)
            cat > .config << 'EOF'
# OSx2 Kernel Configuration
BUILD_QEMU=0
BUILD_IMG=1
BUILD_ISO=0
EOF
            echo "✓ Created .config (Disk)"
            ;;
        3)
            cat > .config << 'EOF'
# OSx2 Kernel Configuration
BUILD_QEMU=0
BUILD_IMG=0
BUILD_ISO=1
EOF
            echo "✓ Created .config (ISO)"
            ;;
        4)
            echo "Opening .config for editing..."
            cat > .config << 'EOF'
# OSx2 Kernel Configuration
# BUILD_QEMU=1  (Boot in QEMU)
# BUILD_IMG=1   (Create FAT disk image)
# BUILD_ISO=1   (Create ISO image)
BUILD_QEMU=1
BUILD_IMG=0
BUILD_ISO=0
EOF
            ${EDITOR:-nano} .config
            ;;
        *)
            echo "Invalid choice"
            exit 1
            ;;
    esac
else
    echo "Found config files:"
    echo "$found_configs" | nl
    echo ""
    echo "0. Create new config"
    echo ""
    read -p "Select config to use [0-$(echo "$found_configs" | wc -l)]: " choice
    
    if [ "$choice" = "0" ]; then
        cat > .config << 'EOF'
# OSx2 Kernel Configuration
BUILD_QEMU=1
BUILD_IMG=0
BUILD_ISO=0
EOF
        echo "✓ Created new .config"
    else
        selected=$(echo "$found_configs" | sed -n "${choice}p")
        if [ -n "$selected" ]; then
            cp "$selected" .config
            echo "✓ Using $selected"
        else
            echo "Invalid selection"
            exit 1
        fi
    fi
fi

echo ""
echo "Configuration ready. Run: make"
