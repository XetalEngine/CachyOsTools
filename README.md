# CachyOsTools 🚀

<div align="center">
  <img src="images/logo.png" alt="CachyOsTools Logo" width="200">
  <br>
  <em>A comprehensive system management tool for CachyOS (tested) and other Arch-based distributions.</em>
</div>

### 🎨 Logo Gallery

<div align="center">
  <img src="images/logo.png" alt="Logo 1" width="150" style="margin: 10px;">
  <img src="images/logo2.png" alt="Logo 2" width="150" style="margin: 10px;">
  <img src="images/logo3.png" alt="Logo 3" width="150" style="margin: 10px;">
  <img src="images/logo4.png" alt="Logo 4" width="150" style="margin: 10px;">
  <img src="images/logo5.png" alt="Logo 5" width="150" style="margin: 10px;">
  <img src="images/logo6.png" alt="Logo 6" width="150" style="margin: 10px;">
  <img src="images/logo7.png" alt="Logo 7" width="150" style="margin: 10px;">
  <img src="images/logo8.png" alt="Logo 8" width="150" style="margin: 10px;">
  <img src="images/logo9.png" alt="Logo 9" width="150" style="margin: 10px;">
</div>

![CachyOsTools](https://img.shields.io/badge/CachyOsTools-System%20Manager-blue)
![Qt](https://img.shields.io/badge/Qt-6%20%7C%205-green)
![Arch](https://img.shields.io/badge/Arch-Based-Supported-orange)

## ✨ Features

- **Drive Management** - Mount, unmount, and manage storage devices
- **System Tweaks** - Performance optimizations and system configurations
- **Service Management** - Start, stop, and configure system services
- **Backup & Restore** - System backup and restoration tools
- **GPU Passthrough** - KVM GPU passthrough configuration
- **System Monitoring** - Log viewing and system diagnostics

## 📦 Dependencies

Install the required dependencies:

```bash
sudo pacman -S qt6-base cmake gcc make
```

**Optional dependencies for full functionality:**
```bash
sudo pacman -S udisks2 systemd-analyze rsync
```

## 🛠️ Building

### Quick Build
```bash
git clone https://github.com/XetalEngine/CachyOsTools.git
cd CachyOsTools
chmod +x build.sh
./build.sh
```

### Manual Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 🚀 Running

After building, run the application:
```bash
./CachyOsTools
```

## 📋 Requirements

- **OS**: CachyOS (tested), other Arch-based distributions (untested)
- **Qt**: Qt6 or Qt5
- **Compiler**: GCC with C++17 support
- **Build Tools**: CMake 3.16+

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 📄 License

This project is licensed under the MIT License.

---

**Made with ❤️ for the CachyOS community** 