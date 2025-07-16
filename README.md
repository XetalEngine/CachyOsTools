# CachyOsTools - Linux System Manager

<div align="center">
  <img src="images/logo.png" alt="CachyOsTools Logo">
  <br>
  <em>A comprehensive Qt-based GUI application for managing CachyOS and other Linux systems</em>
</div>

A comprehensive Qt-based GUI application for managing CachyOS and other Linux systems. This tool provides an intuitive interface for common system administration tasks, making it easier for users to manage their Linux systems without extensive command-line knowledge.

## 📋 Prerequisites

Before building CachyOsTools, you need to install the following packages:

```bash
sudo pacman -S qt6-base cmake ninja
```

### Optional but Recommended Packages:
```bash
sudo pacman -S qt6-tools qt6-declarative
```

## 🚀 Building the Project

### Quick Build (Debug)
```bash
./build.sh
```

### Build with Release Configuration
```bash
./build.sh Release
```

### Manual Build Process
If you prefer to build manually:

1. **Create build directory:**
   ```bash
   mkdir -p build/Desktop-Debug
   cd build/Desktop-Debug
   ```

2. **Configure with CMake:**
   ```bash
   cmake ../.. -DCMAKE_BUILD_TYPE=Debug -G Ninja
   ```

3. **Build the project:**
   ```bash
   ninja
   ```

## 🏃‍♂️ Running the Application

### Debug Build
```bash
./run.sh
```

### Release Build
```bash
./runr.sh
```

### Direct Execution
```bash
./build/Desktop-Debug/CachyOsTools
```

## 🧹 Cleaning Build Files

If you encounter build issues or want to start fresh:

```bash
rm -rf build/
./build.sh
```

## 🛠️ Features

### Drive Management
- **Mount/Unmount Drives**: Easily mount and unmount storage devices
- **Force Mount**: Mount drives with specific options
- **Drive Information**: View detailed information about storage devices
- **Ownership Management**: Change ownership of mounted drives
- **777 Permissions**: Set read/write/execute permissions for all users

### System Services
- **Service Management**: Start, stop, restart, enable, and disable system services
- **User Services**: Toggle between system and user services
- **Service Search**: Find specific services quickly
- **Service Status**: View detailed service information

### Shell Alias Management
- **Auto-Detection**: Automatically detects your shell (bash, zsh, fish, etc.)
- **Alias Management**: Add, edit, and remove shell aliases
- **Multi-Shell Support**: Works with bash, zsh, fish, tcsh, csh, and ksh
- **Configuration Files**: Direct editing of shell configuration files

### System Configuration
- **Config File Editor**: Quick access to common system configuration files
- **Categorized Files**: Organized by category (System, User, Network, etc.)
- **Direct Editing**: Open configuration files in your preferred editor
- **CachyOS Specific**: Special configurations for CachyOS systems

### Log Management
- **System Logs**: View and manage system log files
- **Log Filtering**: Apply filters to log content
- **Log Operations**: Clear, copy, and save log files
- **Real-time Monitoring**: Monitor log files in real-time

### System Tweaks
- **ZRAM Configuration**: Enable/disable and configure ZRAM
- **CPU Governor**: Change CPU performance governors
- **IPv6 Management**: Enable/disable IPv6
- **TRIM Support**: Configure SSD TRIM support
- **TMPFS Configuration**: Manage /tmp filesystem
- **Hidden Files**: Show/hide hidden files in file managers
- **Security Mitigations**: Configure kernel security mitigations

### Backup & Restore
- **System Backup**: Create complete system backups
- **Selective Backup**: Choose specific directories to backup
- **Restore Functionality**: Restore from backup files
- **Drive Selection**: Choose source and destination drives
- **Progress Tracking**: Monitor backup and restore progress

### ISO Creator (Planned)
- **Custom ISO Creation**: Create custom CachyOS ISOs
- **Kernel Selection**: Choose which kernels to include
- **Software Packages**: Select additional software to include
- **Custom Configurations**: Include custom system configurations

## 🎯 Target Audience

- **Linux Beginners**: Users new to Linux who want a GUI for system management
- **CachyOS Users**: Specific optimizations and configurations for CachyOS
- **System Administrators**: Quick access to common administrative tasks
- **Power Users**: Convenient interface for frequent system operations

## 🔧 Technical Details

- **Framework**: Qt6 with C++
- **Build System**: CMake with Ninja
- **Architecture**: Modern C++17
- **UI**: Qt Widgets with custom styling
- **Platform**: Linux (primarily Arch-based distributions)

## 🐛 Troubleshooting

### Build Issues
1. **CMake not found**: Install cmake package
2. **Qt6 not found**: Install qt6-base package
3. **Ninja not found**: Install ninja package
4. **Hardcoded paths error**: Run `rm -rf build/` and rebuild

### Runtime Issues
1. **Permission denied**: Ensure you have sudo privileges for system operations
2. **Services not showing**: Check if systemd is running
3. **Logs not accessible**: Run with appropriate permissions

### Common Solutions
- **Clean rebuild**: `rm -rf build/ && ./build.sh`
- **Update packages**: `sudo pacman -Syu`
- **Check dependencies**: `ldd build/Desktop-Debug/CachyOsTools`

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is open source. Please check the license file for details.

## 🙏 Acknowledgments

- **CachyOS Team**: For the excellent Linux distribution
- **Qt Development Team**: For the powerful Qt framework
- **Linux Community**: For the amazing open-source ecosystem

---

**Note**: This application requires appropriate permissions for system operations. Some features may require sudo privileges to function properly. 