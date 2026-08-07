
// KVM tab guides: Single GPU sub-tab (full passthrough guide) and Dual GPU
// sub-tab (concept guide above the functional 7-step grid).
// Pure documentation for now — the 1-Click KVM sub-tab will automate this later.
// The guides are intentionally English-only (like the Dual GPU step dialogs).

void MainWindow::setupSingleGpuGuide() {
    // Rendered once; QTextBrowser handles scrolling, selection and links.
    ui->singleGpuGuideBrowser->setOpenExternalLinks(true);
    ui->singleGpuGuideBrowser->setHtml(R"HTML(
<style>
  h2 { color:#27ae60; }
  h3 { color:#2ecc71; }
  pre { background-color:#14161a; color:#9cdcfe; padding:8px; }
  td { padding:4px 10px; }
</style>

<h2>🎯 Single GPU Passthrough — the complete guide</h2>
<p><b>What this is:</b> your machine has <i>one</i> GPU, and you want a VM (usually Windows) to use it at
near-native performance. Since the host can't share it, the host <b>gives the GPU away</b> when the VM starts
(your Linux desktop shuts down) and <b>takes it back</b> when the VM stops (your desktop comes back).
Everything is done with libvirt <i>hook scripts</i> — no dual boot, no second GPU needed.</p>

<p style="color:#e67e22;"><b>⚠ The honest trade-off:</b> while the VM runs, the Linux host is headless.
If a hook script is wrong, the desktop may not come back and you'll need SSH or a reboot. Read step 7 twice.</p>

<hr>

<h3>0️⃣ Requirements</h3>
<ul>
<li>CPU with virtualization + IOMMU: Intel <b>VT-x&nbsp;&amp;&nbsp;VT-d</b>, or AMD <b>SVM&nbsp;&amp;&nbsp;AMD-Vi</b> (any Ryzen)</li>
<li>UEFI motherboard with the GPU in its own IOMMU group (checked in step 3)</li>
<li>A second machine or phone is handy for reading this guide while the host is headless</li>
</ul>

<h3>1️⃣ BIOS settings</h3>
<ul>
<li>Enable <b>Intel VT-d</b> / <b>AMD-Vi (IOMMU)</b> — often under "Advanced → CPU" or "Chipset"</li>
<li>Enable <b>VT-x</b> / <b>SVM</b> (virtualization itself)</li>
<li>Boot in pure <b>UEFI</b> mode (disable CSM) — OVMF guests want UEFI GPUs</li>
<li>NVIDIA + Above 4G Decoding / Resizable BAR: usually fine on modern drivers; if the VM later
shows a black screen, try disabling ReBAR first</li>
</ul>

<h3>2️⃣ Enable IOMMU in GRUB</h3>
<p>Edit <code>/etc/default/grub</code> and add to <code>GRUB_CMDLINE_LINUX_DEFAULT</code>:</p>
<table border="0" cellspacing="0" bgcolor="#1d1f24">
<tr><td><b>Intel CPU</b></td><td><code>intel_iommu=on iommu=pt</code></td></tr>
<tr><td><b>AMD CPU</b></td><td><code>amd_iommu=on iommu=pt</code></td></tr>
</table>
<p><code>iommu=pt</code> (passthrough mode) skips IOMMU translation for host devices — free performance.
On AMD, IOMMU is on by default, but being explicit never hurts. Then regenerate GRUB and reboot:</p>
<pre>sudo grub-mkconfig -o /boot/grub/grub.cfg
sudo reboot</pre>

<h3>3️⃣ Verify IOMMU works and find your GPU's group</h3>
<pre>sudo dmesg | grep -i -e DMAR -e IOMMU     # expect "IOMMU enabled" / "AMD-Vi"</pre>
<p>List every IOMMU group and what's inside it:</p>
<pre>#!/bin/bash
for g in /sys/kernel/iommu_groups/*; do
  echo "IOMMU Group ${g##*/}:"
  for d in "$g"/devices/*; do
    echo -e "\t$(lspci -nns "${d##*/}")"
  done
done</pre>
<p>Find your GPU. It usually has <b>two functions</b>: video (<code>01:00.0</code>) and audio
(<code>01:00.1</code>). <b>Both</b> must be passed through, and the group should contain <i>only</i>
the GPU functions (a PCI bridge above them is fine). Note the vendor:device IDs, e.g.
<code>10de:2684</code> and <code>10de:22ba</code>.</p>
<p style="color:#e67e22;">If unrelated devices share the group, move the GPU to another slot or research the
ACS override patch (understand its isolation risks first — CachyOS kernels ship it built in).</p>

<h3>4️⃣ Install the virtualization stack</h3>
<pre>sudo pacman -S qemu-full libvirt virt-manager edk2-ovmf swtpm dnsmasq
sudo systemctl enable --now libvirtd
sudo usermod -aG libvirt $USER      # log out/in afterwards</pre>

<h3>5️⃣ Create the VM (without the GPU yet)</h3>
<ul>
<li>In <b>virt-manager</b>: New VM → your Windows ISO → check <b>"Customize configuration before install"</b></li>
<li>Chipset <b>Q35</b>, Firmware <b>UEFI (OVMF)</b> — for Windows 11 pick the <code>secboot</code> OVMF and add a <b>TPM</b> (emulated, swtpm)</li>
<li>Disk & NIC: <b>VirtIO</b> — attach the
<a href="https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/latest-virtio/virtio-win.iso">virtio-win driver ISO</a>
as a second CD-ROM (Windows needs it to even see the disk)</li>
<li>Install Windows normally through the virtual display (Spice), install virtio drivers, <b>then</b> continue below</li>
</ul>

<h3>6️⃣ vBIOS ROM (mainly NVIDIA)</h3>
<p>Passing a ROM file avoids the initialization quirks of a GPU that the host already booted with:</p>
<pre>cd /sys/bus/pci/devices/0000:01:00.0/
echo 1 | sudo tee rom
sudo cat rom > /tmp/gpu.rom
echo 0 | sudo tee rom</pre>
<p>If the dump fails, download your exact card's vBIOS from TechPowerUp's database.
<b>NVIDIA only:</b> open the ROM in a hex editor and delete everything <i>before</i> the value
<code>55 AA</code> that is followed by the ASCII string <code>VIDEO</code> — that header confuses OVMF.
Save as <code>patched.rom</code>, put it in <code>/var/lib/libvirt/vbios/</code>, and reference it later in the
GPU's XML: <code>&lt;rom file="/var/lib/libvirt/vbios/patched.rom"/&gt;</code>. AMD cards normally need no ROM file.</p>

<h3>7️⃣ The hook scripts — where the magic happens</h3>
<p>Libvirt runs <code>/etc/libvirt/hooks/qemu</code> on VM events. The dispatcher below runs scripts from
per-VM folders (replace <code>win10</code> with your VM's name everywhere):</p>
<pre># /etc/libvirt/hooks/qemu          (chmod +x)
#!/bin/bash
GUEST="$1"; OP="$2"
BASE="/etc/libvirt/hooks/qemu.d/$GUEST"
if [ "$OP" = "prepare" ];  then "$BASE/prepare/begin/start.sh"  || exit 1; fi
if [ "$OP" = "release" ];  then "$BASE/release/end/revert.sh"   || true;  fi</pre>

<p><b>start.sh</b> — host lets go of the GPU (example for NVIDIA on KDE/SDDM):</p>
<pre># /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh   (chmod +x)
#!/bin/bash
set -x
# 1. Stop the display manager (kills your desktop!)
systemctl stop sddm                    # gdm / lightdm on other setups
# 2. Unbind virtual consoles + EFI framebuffer
echo 0 > /sys/class/vtconsole/vtcon0/bind
echo 0 > /sys/class/vtconsole/vtcon1/bind
echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind
sleep 2
# 3. Unload GPU drivers  (AMD: just "modprobe -r amdgpu")
modprobe -r nvidia_drm nvidia_modeset nvidia_uvm nvidia
# 4. Detach the GPU from the host (both functions!)
virsh nodedev-detach pci_0000_01_00_0
virsh nodedev-detach pci_0000_01_00_1
# 5. Load VFIO
modprobe vfio vfio_pci vfio_iommu_type1</pre>

<p><b>revert.sh</b> — host takes the GPU back after shutdown:</p>
<pre># /etc/libvirt/hooks/qemu.d/win10/release/end/revert.sh   (chmod +x)
#!/bin/bash
set -x
modprobe -r vfio_pci vfio_iommu_type1 vfio
virsh nodedev-reattach pci_0000_01_00_0
virsh nodedev-reattach pci_0000_01_00_1
echo 1 > /sys/class/vtconsole/vtcon0/bind
echo 1 > /sys/class/vtconsole/vtcon1/bind
modprobe nvidia_drm nvidia_modeset nvidia_uvm nvidia   # AMD: modprobe amdgpu
systemctl start sddm</pre>
<p style="color:#e67e22;">⚠ Adjust <code>01:00.x</code> to your PCI addresses from step 3, and
<code>sddm</code> to your display manager. Test each command manually over SSH once before trusting the scripts.</p>

<h3>8️⃣ Give the VM the GPU and your input devices</h3>
<ul>
<li>virt-manager → Add Hardware → <b>PCI Host Device</b> → add <b>both</b> GPU functions (01:00.0 and 01:00.1)</li>
<li>Add your <b>USB keyboard and mouse</b> the same way (USB Host Device), or use evdev forwarding for
hotkey-switchable input</li>
<li><b>Remove</b> the virtual Display Spice, Video QXL, and emulated tablet — with a real GPU attached they
cause black screens and cursor weirdness</li>
<li>The monitor cable stays in the GPU — the VM's output appears on your real monitor</li>
</ul>

<h3>9️⃣ Audio</h3>
<p>Simplest: HDMI/DP audio through the monitor (the GPU's audio function is already passed).
Alternatively route guest audio to the host's PipeWire later — start simple.</p>

<h3>🔟 Performance polish (optional but worth it)</h3>
<pre>&lt;vcpu placement="static"&gt;12&lt;/vcpu&gt;
&lt;cputune&gt;
  &lt;vcpupin vcpu="0" cpuset="4"/&gt;   &lt;!-- pin VM cores to one CCD, --&gt;
  &lt;vcpupin vcpu="1" cpuset="5"/&gt;   &lt;!-- leave cores 0-3 for the host --&gt;
  ...
&lt;/cputune&gt;
&lt;cpu mode="host-passthrough"&gt;
  &lt;topology sockets="1" dies="1" cores="6" threads="2"/&gt;
  &lt;cache mode="passthrough"/&gt;
&lt;/cpu&gt;</pre>
<ul>
<li>On a 7950X3D: pin the VM to the X3D CCD for games; check core numbering with <code>lscpu -e</code></li>
<li>Hugepages: <code>&lt;memoryBacking&gt;&lt;hugepages/&gt;&lt;/memoryBacking&gt;</code> (the Dual GPU tab's Step 7 sets them up)</li>
<li>Disk: VirtIO + <code>cache=none</code>, or pass a whole NVMe as a PCI device for max speed</li>
</ul>

<h3>🚑 Troubleshooting</h3>
<table border="0" cellspacing="0" bgcolor="#1d1f24">
<tr><td><b>Black screen in VM</b></td><td>ROM file missing/unpatched (step 6), virtual Display/Video not removed (step 8), or ReBAR — try disabling it</td></tr>
<tr><td><b>Desktop never comes back</b></td><td>revert.sh failed — SSH in and run its commands by hand; usual culprit: driver modules refuse to load while something holds the GPU</td></tr>
<tr><td><b>Error 43 (NVIDIA)</b></td><td>Historical on old drivers; if seen, add &lt;hidden state="on"/&gt; in &lt;kvm&gt; and a &lt;vendor_id&gt; in hyperv features</td></tr>
<tr><td><b>AMD "reset bug"</b></td><td>Polaris/Vega/Navi cards may need the <b>vendor-reset</b> kernel module to survive VM restarts</td></tr>
<tr><td><b>Group not isolated</b></td><td>Different PCIe slot, or ACS override (see step 3)</td></tr>
</table>

<h3>📚 References</h3>
<ul>
<li><a href="https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF">Arch Wiki — PCI passthrough via OVMF</a> (the bible)</li>
<li><a href="https://github.com/joeknock90/Single-GPU-Passthrough">joeknock90/Single-GPU-Passthrough</a></li>
<li><a href="https://gitlab.com/risingprismtv/single-gpu-passthrough">risingprismtv/single-gpu-passthrough</a></li>
<li><a href="https://github.com/gnif/vendor-reset">gnif/vendor-reset</a> (AMD reset bug)</li>
</ul>
<p style="color:#888;">The ⚡ 1-Click KVM tab will eventually automate all of this — until then, this page is the map.</p>
)HTML");
}

void MainWindow::setupDualGpuGuide() {
    ui->dualGpuGuideBrowser->setOpenExternalLinks(true);
    ui->dualGpuGuideBrowser->setHtml(R"HTML(
<style>
  h2 { color:#27ae60; }
  h3 { color:#2ecc71; }
  pre { background-color:#14161a; color:#9cdcfe; padding:8px; }
  td { padding:4px 10px; }
</style>

<h2>🖥️ Dual GPU Passthrough — keep your desktop, give the VM its own GPU</h2>
<p><b>The idea:</b> the host keeps one GPU for your Linux desktop, the VM permanently owns the other.
No hook scripts, no headless host, no risk of losing your session — this is the <i>comfortable</i> way to
do passthrough, and it unlocks <b>Looking Glass</b> (the VM's screen inside a window on your desktop).</p>

<table border="0" cellspacing="0" bgcolor="#1d1f24">
<tr><td><b>Setup A — iGPU + dGPU</b> <span style="color:#27ae60;">(most common)</span></td>
    <td>Intel iGPU or AMD APU graphics runs the Linux desktop; the discrete card goes to the VM</td></tr>
<tr><td><b>Setup B — two discrete GPUs</b></td>
    <td>A small/older card for the host, the big one for the VM</td></tr>
</table>

<h3>🔧 iGPU specifics (Setup A)</h3>
<ul>
<li><b>BIOS:</b> enable <b>iGPU Multi-Monitor</b> (keeps the iGPU active with a dGPU present) and set
<b>Primary Display → IGD/iGPU</b> so the host boots on it</li>
<li>Plug your monitor into the <b>motherboard's</b> video output (or a second input of the same monitor —
switch inputs to look at the VM directly)</li>
<li>Drivers are automatic: Intel → <code>i915</code>, AMD APU → <code>amdgpu</code>. Nothing to install on CachyOS</li>
<li>7000-series Ryzen (like the 7950X3D) has RDNA2 graphics — plenty for a desktop, browsers and video</li>
</ul>

<h3>🗺️ How the steps below map to the plan</h3>
<p>The buttons under this guide do the real work — each has <b>Info</b>, <b>Backup</b> and <b>Edit</b>.
This is what each step achieves:</p>
<table border="0" cellspacing="0" bgcolor="#1d1f24">
<tr><td><b>Step 1 — Xorg config</b></td><td>Pins the Linux desktop to the host GPU/iGPU so it never touches the VM's card</td></tr>
<tr><td><b>Step 2 — GRUB IOMMU</b></td><td><code>intel_iommu=on iommu=pt</code> (Intel) / <code>amd_iommu=on iommu=pt</code> (AMD) — enables device isolation</td></tr>
<tr><td><b>Step 3 — Verify</b></td><td>Confirms IOMMU is live and shows which group each GPU sits in</td></tr>
<tr><td><b>Step 4 — Blacklist</b></td><td>Stops the host driver (nvidia/amdgpu) from claiming the VM's card at boot</td></tr>
<tr><td><b>Step 5 — Bind to VFIO</b></td><td><code>options vfio-pci ids=xxxx:xxxx,xxxx:xxxx</code> — the VM's GPU (video <b>and</b> audio function!) is claimed by vfio-pci instead</td></tr>
<tr><td><b>Step 6 — Early modules</b></td><td>Loads vfio-pci in the initramfs so it wins the race against the real driver</td></tr>
<tr><td><b>Step 7 — Huge pages</b></td><td>Faster VM memory — allocate enough for your VM's RAM</td></tr>
</table>

<h3>⚠️ The two classic gotchas</h3>
<ul>
<li><b style="color:#e67e22;">Identical GPUs:</b> <code>ids=</code> binding matches vendor:device IDs — with two
identical cards it grabs <i>both</i>. Bind by PCI address instead with a small script in the initramfs
(<code>driver_override</code> on <code>/sys/bus/pci/devices/0000:01:00.0</code>) — see the Arch Wiki link below</li>
<li><b style="color:#e67e22;">Wrong card grabbed:</b> after reboot check with
<code>lspci -nnk</code> — the VM's GPU must show <code>Kernel driver in use: vfio-pci</code>,
and the host's GPU its normal driver</li>
</ul>

<h3>🚀 After the steps: build the VM</h3>
<ul>
<li>VM creation, virtio drivers, vBIOS notes, CPU pinning and troubleshooting are identical to the
<b>🎯 Single GPU</b> tab — follow its steps 4-6 and 10, <i>skip its hook-script section entirely</i> (that's the luxury of two GPUs)</li>
<li>Add both functions of the VM's GPU as PCI Host Devices in virt-manager; keep or remove the virtual
display depending on whether you use Looking Glass or a real monitor input</li>
</ul>

<h3>🪟 Looking Glass (the dual-GPU superpower)</h3>
<p>The VM renders on its GPU, and you see the output in a <i>window</i> on your Linux desktop —
near-zero latency, no monitor switching:</p>
<pre>« in the VM XML »        &lt;shmem name="looking-glass"&gt;&lt;model type="ivshmem-plain"/&gt;
                         &lt;size unit="M"&gt;64&lt;/size&gt;&lt;/shmem&gt;
« host »                 pacman -S looking-glass        # client
« Windows guest »        install the Looking Glass host service + IddSampleDriver</pre>
<p>Size the shmem for your resolution (64M covers 4K SDR). Pair it with evdev or a
<a href="https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Passing_keyboard/mouse_via_Evdev">shared keyboard/mouse</a> for a seamless setup.</p>

<h3>📚 References</h3>
<ul>
<li><a href="https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF">Arch Wiki — PCI passthrough via OVMF</a> (incl. identical-GPU driver_override)</li>
<li><a href="https://looking-glass.io/docs/">Looking Glass documentation</a></li>
</ul>
<p style="color:#888;">Work through Steps 1→7 below in order — every one has a Backup button for a reason.</p>
)HTML");
}
