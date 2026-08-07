<?xml version='1.0' encoding='utf-8'?>
<TS version="2.1" language="pt_BR">
<context>
    <name>MainWindow</name>
    <message>
        <source>Arch OS Tools</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Dashboard</source>
        <translation>Painel</translation>
    </message>
    <message>
        <source>🏠 Welcome</source>
        <translation>🏠 Início</translation>
    </message>
    <message>
        <source>System at a glance</source>
        <translation>Visão geral do sistema</translation>
    </message>
    <message>
        <source>Language / Idioma / Sprache</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>🔄 Refresh</source>
        <translation>🔄 Atualizar</translation>
    </message>
    <message>
        <source>💾 Drive Health</source>
        <translation>💾 Saúde dos Discos</translation>
    </message>
    <message>
        <source>Loading...</source>
        <translation>Carregando...</translation>
    </message>
    <message>
        <source>Open →</source>
        <translation>Abrir →</translation>
    </message>
    <message>
        <source>🧩 Services</source>
        <translation>🧩 Serviços</translation>
    </message>
    <message>
        <source>⬆️ Updates</source>
        <translation>⬆️ Atualizações</translation>
    </message>
    <message>
        <source>📊 Disk Space</source>
        <translation>📊 Espaço em Disco</translation>
    </message>
    <message>
        <source>📀 Safety ISO</source>
        <translation>📀 ISO de Segurança</translation>
    </message>
    <message>
        <source>🧠 System</source>
        <translation>🧠 Sistema</translation>
    </message>
    <message>
        <source>🔌 Devices</source>
        <translation>🔌 Dispositivos</translation>
    </message>
    <message>
        <source>👥 Users &amp; Groups</source>
        <translation>👥 Usuários e Grupos</translation>
    </message>
    <message>
        <source>⏪ Restore Points</source>
        <translation>⏪ Pontos de Restauração</translation>
    </message>
    <message>
        <source>1) Exact Clone (default) — restore the system exactly as it is. Flawless on same/similar hardware. ❓</source>
        <translation>1) Clone Exato (padrão) — restaura o sistema exatamente como está. Perfeito em hardware igual/semelhante. ❓</translation>
    </message>
    <message>
        <source>A byte-for-byte restore: same users, same SSH keys, same machine-id.
Everything bound to the machine identity keeps working — including systemd
encrypted credentials (e.g. hardened libvirt secret setups).
The right choice for disaster recovery onto the same or similar hardware.</source>
        <translation>Uma restauração idêntica: mesmos usuários, mesmas chaves SSH, mesmo machine-id.
Tudo que depende da identidade da máquina continua funcionando — inclusive
credenciais criptografadas do systemd (ex.: setups de segredos do libvirt).
A escolha certa para recuperação de desastre no mesmo hardware ou similar.</translation>
    </message>
    <message>
        <source>    a) Auto-detect network cards — repair bridges &amp; adapter bindings on first boot ❓</source>
        <translation>    a) Autodetectar placas de rede — reparar bridges e vínculos de adaptadores no primeiro boot ❓</translation>
    </message>
    <message>
        <source>On first boot, re-detects the network adapters actually present and re-binds
NetworkManager profiles and bridges (br0 etc.) to them.
Pick this when the target machine has different network hardware.</source>
        <translation>No primeiro boot, redetecta os adaptadores de rede presentes e revincula
os perfis do NetworkManager e as bridges (br0 etc.) a eles.
Escolha quando a máquina de destino tiver hardware de rede diferente.</translation>
    </message>
    <message>
        <source>    b) Auto-detect GPU — NVIDIA: leave untouched; AMD: install drivers + configure X11 &amp; Wayland ❓</source>
        <translation>    b) Autodetectar GPU — NVIDIA: não mexer; AMD: instalar drivers + configurar X11 e Wayland ❓</translation>
    </message>
    <message>
        <source>On first boot, detects the GPU. NVIDIA cards are left untouched (your drivers stay).
AMD cards get drivers installed and X11 &amp; Wayland configured.
Pick this when the target machine has a different graphics card.</source>
        <translation>No primeiro boot, detecta a GPU. Placas NVIDIA ficam intocadas (seus drivers permanecem).
Placas AMD recebem drivers e configuração de X11 e Wayland.
Escolha quando a máquina de destino tiver outra placa de vídeo.</translation>
    </message>
    <message>
        <source>    c) Offer username &amp; password change on first boot ❓</source>
        <translation>    c) Oferecer troca de usuário e senha no primeiro boot ❓</translation>
    </message>
    <message>
        <source>On first boot, offers to rename the user account and set a new password —
useful when handing the cloned system to someone else.
Home directory and group memberships are migrated to the new name.</source>
        <translation>No primeiro boot, oferece renomear a conta de usuário e definir nova senha —
útil ao entregar o sistema clonado para outra pessoa.
O diretório home e os grupos são migrados para o novo nome.</translation>
    </message>
    <message>
        <source>    d) Regenerate SSH host keys ❓</source>
        <translation>    d) Regenerar chaves SSH do host ❓</translation>
    </message>
    <message>
        <source>Deletes /etc/ssh/ssh_host_* and generates fresh keys on first boot, so the clone
does not impersonate the original machine over SSH.
People connecting will see a one-time "host key changed" warning — that is expected.</source>
        <translation>Apaga /etc/ssh/ssh_host_* e gera chaves novas no primeiro boot, para que o clone
não se passe pela máquina original via SSH.
Quem conectar verá um aviso único de "host key changed" — isso é esperado.</translation>
    </message>
    <message>
        <source>    e) Regenerate machine-id (new systemd identity) — ⚠ read the tooltip! ❓</source>
        <translation>    e) Regenerar machine-id (nova identidade systemd) — ⚠ leia o tooltip! ❓</translation>
    </message>
    <message>
        <source>⚠ Gives the install a brand-new /etc/machine-id — applied during installation.

BREAKS anything bound to the old identity, most notably systemd ENCRYPTED
CREDENTIALS (LoadCredentialEncrypted): services like hardened libvirt secret
setups will fail with decryption errors until their credentials are re-created.
(A stale libvirt secrets-encryption-key is removed automatically so its init
service can regenerate it — but secrets encrypted under the old key are lost.)
Journald history also splits at the identity change.

Leave OFF for exact restores. Turn ON when the clone will live on the same
network as the original long-term (avoids duplicate DHCP/systemd identity).</source>
        <translation>⚠ Dá à instalação um /etc/machine-id totalmente novo — aplicado durante a instalação.

QUEBRA tudo que estiver vinculado à identidade antiga, principalmente CREDENCIAIS
CRIPTOGRAFADAS do systemd (LoadCredentialEncrypted): serviços como setups de
segredos do libvirt falharão com erros de descriptografia até recriar as credenciais.
(Uma secrets-encryption-key antiga do libvirt é removida automaticamente para o
serviço de init regenerá-la — mas segredos criptografados com a chave antiga são perdidos.)
O histórico do journald também se divide na troca de identidade.

Deixe DESLIGADO para restaurações exatas. Ligue quando o clone for viver na mesma
rede que o original por muito tempo (evita identidade DHCP/systemd duplicada).</translation>
    </message>
    <message>
        <source>🛡️ Firewall</source>
        <translation>🛡️ Firewall</translation>
    </message>
    <message>
        <source>🧬 Kernels</source>
        <translation>🧬 Kernels</translation>
    </message>
    <message>
        <source>🌱 Environment</source>
        <translation>🌱 Ambiente</translation>
    </message>
    <message>
        <source>🪪 System Properties</source>
        <translation>🪪 Propriedades do Sistema</translation>
    </message>
    <message>
        <source>Drives</source>
        <translation>Discos</translation>
    </message>
    <message>
        <source>Filters</source>
        <translation>Filtros</translation>
    </message>
    <message>
        <source>Disk</source>
        <translation>Disco</translation>
    </message>
    <message>
        <source>Partition</source>
        <translation>Partição</translation>
    </message>
    <message>
        <source>Min Size</source>
        <translation>Tam. Mínimo</translation>
    </message>
    <message>
        <source> MB</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Available Drives &amp; Partitions</source>
        <translation>Discos e Partições Disponíveis</translation>
    </message>
    <message>
        <source>🏷️ Load Labels</source>
        <translation>🏷️ Carregar Rótulos</translation>
    </message>
    <message>
        <source>Read partition labels directly from the filesystems (asks for sudo once). Use when the Label column shows dashes.</source>
        <translation>Lê os rótulos das partições diretamente dos sistemas de arquivos (pede sudo uma vez). Use quando a coluna Rótulo mostrar traços.</translation>
    </message>
    <message>
        <source>📁 Mount</source>
        <translation>📁 Montar</translation>
    </message>
    <message>
        <source>⚡ Force Mount</source>
        <translation>⚡ Forçar Montagem</translation>
    </message>
    <message>
        <source>📤 Unmount</source>
        <translation>📤 Desmontar</translation>
    </message>
    <message>
        <source>Mount 777</source>
        <translation>Montar 777</translation>
    </message>
    <message>
        <source>Take Ownership</source>
        <translation>Assumir Propriedade</translation>
    </message>
    <message>
        <source>⏏️ Eject</source>
        <translation>⏏️ Ejetar</translation>
    </message>
    <message>
        <source>🔧 Format</source>
        <translation>🔧 Formatar</translation>
    </message>
    <message>
        <source>📊 SMART Info</source>
        <translation>📊 Info SMART</translation>
    </message>
    <message>
        <source>❤️ Health Check</source>
        <translation>❤️ Verificação de Saúde</translation>
    </message>
    <message>
        <source>🔥 Burn ISO → USB</source>
        <translation>🔥 Gravar ISO → USB</translation>
    </message>
    <message>
        <source>Command Output</source>
        <translation>Saída do Comando</translation>
    </message>
    <message>
        <source>Command output will appear here...</source>
        <translation>A saída do comando aparecerá aqui...</translation>
    </message>
    <message>
        <source>Shell</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Detected Shell: </source>
        <translation>Shell Detectado: </translation>
    </message>
    <message>
        <source>Add Alias</source>
        <translation>Adicionar Alias</translation>
    </message>
    <message>
        <source>Edit Alias</source>
        <translation>Editar Alias</translation>
    </message>
    <message>
        <source>Remove Alias</source>
        <translation>Remover Alias</translation>
    </message>
    <message>
        <source>Reload</source>
        <translation>Recarregar</translation>
    </message>
    <message>
        <source>Services</source>
        <translation>Serviços</translation>
    </message>
    <message>
        <source>Search services...</source>
        <translation>Pesquisar serviços...</translation>
    </message>
    <message>
        <source>Show User Services</source>
        <translation>Mostrar Serviços do Usuário</translation>
    </message>
    <message>
        <source>⏱️ Boot Analysis</source>
        <translation>⏱️ Análise de Boot</translation>
    </message>
    <message>
        <source>Checking failed services...</source>
        <translation>Verificando serviços com falha...</translation>
    </message>
    <message>
        <source>Start</source>
        <translation>Iniciar</translation>
    </message>
    <message>
        <source>Stop</source>
        <translation>Parar</translation>
    </message>
    <message>
        <source>Restart</source>
        <translation>Reiniciar</translation>
    </message>
    <message>
        <source>Enable</source>
        <translation>Habilitar</translation>
    </message>
    <message>
        <source>Disable</source>
        <translation>Desabilitar</translation>
    </message>
    <message>
        <source>Select a service to see its recent journal entries...</source>
        <translation>Selecione um serviço para ver suas entradas recentes no journal...</translation>
    </message>
    <message>
        <source>Tweaks</source>
        <translation>Ajustes</translation>
    </message>
    <message>
        <source>System Tweaks and Optimizations</source>
        <translation>Ajustes e Otimizações do Sistema</translation>
    </message>
    <message>
        <source>Open Config Backups</source>
        <translation>Abrir Backups de Config</translation>
    </message>
    <message>
        <source>Click "Info" to see detailed instructions, "Apply" to execute the tweak, or "Edit Config" to manually edit configuration files.</source>
        <translation>Clique em "Info" para ver instruções detalhadas, "Aplicar" para executar o ajuste ou "Editar Config" para editar manualmente os arquivos de configuração.</translation>
    </message>
    <message>
        <source>⚡ Performance</source>
        <translation>⚡ Desempenho</translation>
    </message>
    <message>
        <source>ZRAM (Compressed RAM Swap)</source>
        <translation>ZRAM (Swap de RAM Comprimida)</translation>
    </message>
    <message>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <source>ℹ️ Info</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>✅ Apply</source>
        <translation>✅ Aplicar</translation>
    </message>
    <message>
        <source>💾 Backup</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>✏️ Edit Config</source>
        <translation>✏️ Editar Config</translation>
    </message>
    <message>
        <source>CPU Governor (Performance/Powersave)</source>
        <translation>Governador de CPU (Desempenho/Economia)</translation>
    </message>
    <message>
        <source>/tmp as tmpfs (RAM Disk)</source>
        <translation>/tmp como tmpfs (Disco RAM)</translation>
    </message>
    <message>
        <source>Performance Hacks (by forAUR)</source>
        <translation>Hacks de Desempenho (por forAUR)</translation>
    </message>
    <message>
        <source>💾 Backup updatedb.conf</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>✏️ Edit /etc/updatedb.conf</source>
        <translation>✏️ Editar /etc/updatedb.conf</translation>
    </message>
    <message>
        <source>💾 Backup mkinitcpio.conf</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>✏️ Edit /etc/mkinitcpio.conf</source>
        <translation>✏️ Editar /etc/mkinitcpio.conf</translation>
    </message>
    <message>
        <source>Gaming Optimizations (vm.max_map_count)</source>
        <translation>Otimizações para Jogos (vm.max_map_count)</translation>
    </message>
    <message>
        <source>CPU Turbo Boost</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>NMI Watchdog (disable = less overhead)</source>
        <translation>NMI Watchdog (desativar = menos overhead)</translation>
    </message>
    <message>
        <source>🔒 Security</source>
        <translation>🔒 Segurança</translation>
    </message>
    <message>
        <source>Spectre/Meltdown Mitigations</source>
        <translation>Mitigações Spectre/Meltdown</translation>
    </message>
    <message>
        <source>ptrace for Non-Root Users</source>
        <translation>ptrace para Usuários Não-Root</translation>
    </message>
    <message>
        <source>hidepid for /proc Security</source>
        <translation>hidepid para Segurança do /proc</translation>
    </message>
    <message>
        <source>Firewall (firewalld/ufw)</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Core Dumps (disable = privacy + disk space)</source>
        <translation>Core Dumps (desativar = privacidade + espaço)</translation>
    </message>
    <message>
        <source>🌐 Network</source>
        <translation>🌐 Rede</translation>
    </message>
    <message>
        <source>IPv6 System-Wide</source>
        <translation>IPv6 no Sistema Todo</translation>
    </message>
    <message>
        <source>DNS Configuration</source>
        <translation>Configuração de DNS</translation>
    </message>
    <message>
        <source>TCP Optimizations</source>
        <translation>Otimizações TCP</translation>
    </message>
    <message>
        <source>💾 Storage</source>
        <translation>💾 Armazenamento</translation>
    </message>
    <message>
        <source>TRIM for SSDs</source>
        <translation>TRIM para SSDs</translation>
    </message>
    <message>
        <source>🖥️ Desktop</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Show Hidden Files by Default</source>
        <translation>Mostrar Arquivos Ocultos por Padrão</translation>
    </message>
    <message>
        <source>⚙️ System</source>
        <translation>⚙️ Sistema</translation>
    </message>
    <message>
        <source>Swappiness (Swap Usage)</source>
        <translation>Swappiness (Uso de Swap)</translation>
    </message>
    <message>
        <source>I/O Scheduler (SSD/NVMe Optimization)</source>
        <translation>Escalonador de E/S (Otimização SSD/NVMe)</translation>
    </message>
    <message>
        <source>Transparent Huge Pages (THP)</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Pacman Optimizations</source>
        <translation>Otimizações do Pacman</translation>
    </message>
    <message>
        <source>Journald Settings (Log Size Limit)</source>
        <translation>Configurações do Journald (Limite de Tamanho de Log)</translation>
    </message>
    <message>
        <source>Memory Cache Tuning (dirty ratio / cache pressure)</source>
        <translation>Ajuste de Cache de Memória (dirty ratio / cache pressure)</translation>
    </message>
    <message>
        <source>Inotify Watch Limits (dev tools / file sync)</source>
        <translation>Limites de Watch do Inotify (ferramentas dev / sync)</translation>
    </message>
    <message>
        <source>KVM</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>🖥️ Dual GPU</source>
        <translation>🖥️ GPU Dupla</translation>
    </message>
    <message>
        <source>KVM Virtual Machine Setup and GPU Passthrough</source>
        <translation>Configuração de VM KVM e GPU Passthrough</translation>
    </message>
    <message>
        <source>Click any option to see detailed instructions for KVM setup and GPU passthrough configuration. This approach is safer as it doesn't automatically modify system files.</source>
        <translation>Clique em qualquer opção para ver instruções detalhadas de configuração de KVM e GPU passthrough. Esta abordagem é mais segura pois não modifica arquivos do sistema automaticamente.</translation>
    </message>
    <message>
        <source>✅ Step 1: Create multi-GPU Xorg config</source>
        <translation>✅ Passo 1: Criar config Xorg multi-GPU</translation>
    </message>
    <message>
        <source>Backup</source>
        <translation>Backup</translation>
    </message>
    <message>
        <source>Backup Xorg config file</source>
        <translation>Backup do arquivo de config do Xorg</translation>
    </message>
    <message>
        <source>Edit /etc/X11/xorg.conf.d/10-hybrid.conf</source>
        <translation>Editar /etc/X11/xorg.conf.d/10-hybrid.conf</translation>
    </message>
    <message>
        <source>✅ Step 2: Configure GRUB for IOMMU</source>
        <translation>✅ Passo 2: Configurar GRUB para IOMMU</translation>
    </message>
    <message>
        <source>Backup GRUB config file</source>
        <translation>Backup do arquivo de config do GRUB</translation>
    </message>
    <message>
        <source>Edit /etc/default/grub</source>
        <translation>Editar /etc/default/grub</translation>
    </message>
    <message>
        <source>✅ Step 3: Verify IOMMU and GPU detection</source>
        <translation>✅ Passo 3: Verificar IOMMU e detecção de GPU</translation>
    </message>
    <message>
        <source>Backup verification results</source>
        <translation>Backup dos resultados de verificação</translation>
    </message>
    <message>
        <source>Run verification commands</source>
        <translation>Executar comandos de verificação</translation>
    </message>
    <message>
        <source>✈️ Step 4: Isolate GPU from Host Using VFIO</source>
        <translation>✈️ Passo 4: Isolar GPU do Host com VFIO</translation>
    </message>
    <message>
        <source>Backup blacklist config</source>
        <translation>Backup da config de blacklist</translation>
    </message>
    <message>
        <source>Edit /etc/modprobe.d/blacklist-nvidia.conf</source>
        <translation>Editar /etc/modprobe.d/blacklist-nvidia.conf</translation>
    </message>
    <message>
        <source>✈️ Step 5: Bind GPU to VFIO</source>
        <translation>✈️ Passo 5: Vincular GPU ao VFIO</translation>
    </message>
    <message>
        <source>Backup VFIO config</source>
        <translation>Backup da config VFIO</translation>
    </message>
    <message>
        <source>Edit /etc/modprobe.d/vfio.conf</source>
        <translation>Editar /etc/modprobe.d/vfio.conf</translation>
    </message>
    <message>
        <source>✈️ Step 6: Ensure VFIO modules load early</source>
        <translation>✈️ Passo 6: Carregar módulos VFIO cedo</translation>
    </message>
    <message>
        <source>Backup modules config</source>
        <translation>Backup da config de módulos</translation>
    </message>
    <message>
        <source>Edit /etc/modules-load.d/vfio.conf</source>
        <translation>Editar /etc/modules-load.d/vfio.conf</translation>
    </message>
    <message>
        <source>💾 Step 7: Configure Huge Pages</source>
        <translation>💾 Passo 7: Configurar Huge Pages</translation>
    </message>
    <message>
        <source>Backup huge pages config</source>
        <translation>Backup da config de huge pages</translation>
    </message>
    <message>
        <source>Configure Huge Pages</source>
        <translation>Configurar Huge Pages</translation>
    </message>
    <message>
        <source>🎯 Single GPU</source>
        <translation>🎯 GPU Única</translation>
    </message>
    <message>
        <source>⚡ 1-Click KVM</source>
        <translation>⚡ KVM em 1 Clique</translation>
    </message>
    <message>
        <source>🚧</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Under Construction</source>
        <translation>Em Construção</translation>
    </message>
    <message>
        <source>One click will do it all: build QEMU &amp; EDK2 from source, set up libvirt, dump &amp; patch your vBIOS, write the hook scripts, and configure single- or dual-GPU passthrough automatically.</source>
        <translation>Um clique fará tudo: compilar QEMU e EDK2 do código-fonte, configurar o libvirt, extrair e corrigir sua vBIOS, criar os hook scripts e configurar o passthrough de GPU única ou dupla automaticamente.</translation>
    </message>
    <message>
        <source>Until then, the Dual GPU and Single GPU tabs walk you through every step manually.</source>
        <translation>Até lá, as abas GPU Dupla e GPU Única te guiam por cada passo manualmente.</translation>
    </message>
    <message>
        <source>System Config</source>
        <translation>Config. do Sistema</translation>
    </message>
    <message>
        <source>Text Editor:</source>
        <translation>Editor de Texto:</translation>
    </message>
    <message>
        <source>nano</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>kate</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>mousepad</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>geany</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>📝 Edit Selected (Built-in, sudo-capable)</source>
        <translation>📝 Editar Selecionado (embutido, com sudo)</translation>
    </message>
    <message>
        <source>Logs</source>
        <translation>Logs</translation>
    </message>
    <message>
        <source>Log File:</source>
        <translation>Arquivo de Log:</translation>
    </message>
    <message>
        <source>🗑️ Clear Log</source>
        <translation>🗑️ Limpar Log</translation>
    </message>
    <message>
        <source>Load the systemd journal for a specific boot</source>
        <translation>Carregar o journal do systemd de um boot específico</translation>
    </message>
    <message>
        <source>Journal: Current Boot</source>
        <translation>Journal: Boot Atual</translation>
    </message>
    <message>
        <source>Journal: Previous Boot</source>
        <translation>Journal: Boot Anterior</translation>
    </message>
    <message>
        <source>Journal: 2 Boots Ago</source>
        <translation>Journal: 2 Boots Atrás</translation>
    </message>
    <message>
        <source>▶ Follow Journal</source>
        <translation>▶ Seguir Journal</translation>
    </message>
    <message>
        <source>Filter:</source>
        <translation>Filtro:</translation>
    </message>
    <message>
        <source>Search in log content...</source>
        <translation>Pesquisar no conteúdo do log...</translation>
    </message>
    <message>
        <source>Lines:</source>
        <translation>Linhas:</translation>
    </message>
    <message>
        <source> lines</source>
        <translation> linhas</translation>
    </message>
    <message>
        <source>Follow (tail)</source>
        <translation>Seguir (tail)</translation>
    </message>
    <message>
        <source>Apply</source>
        <translation>Aplicar</translation>
    </message>
    <message>
        <source>Log content will appear here...</source>
        <translation>O conteúdo do log aparecerá aqui...</translation>
    </message>
    <message>
        <source>Ready</source>
        <translation>Pronto</translation>
    </message>
    <message>
        <source>📋 Copy</source>
        <translation>📋 Copiar</translation>
    </message>
    <message>
        <source>💾 Save</source>
        <translation>💾 Salvar</translation>
    </message>
    <message>
        <source>System Backup Source:</source>
        <translation>Origem do Backup do Sistema:</translation>
    </message>
    <message>
        <source>Source Path:</source>
        <translation>Caminho de Origem:</translation>
    </message>
    <message>
        <source>Destination Drive:</source>
        <translation>Disco de Destino:</translation>
    </message>
    <message>
        <source>Destination Folder:</source>
        <translation>Pasta de Destino:</translation>
    </message>
    <message>
        <source>Select source and destination to begin backup.</source>
        <translation>Selecione origem e destino para iniciar o backup.</translation>
    </message>
    <message>
        <source>Start Backup</source>
        <translation>Iniciar Backup</translation>
    </message>
    <message>
        <source>Source: -\nDestination: -</source>
        <translation>Origem: -\nDestino: -</translation>
    </message>
    <message>
        <source>Restore</source>
        <translation>Restaurar</translation>
    </message>
    <message>
        <source>Backup Source:</source>
        <translation>Origem do Backup:</translation>
    </message>
    <message>
        <source>Backup Path:</source>
        <translation>Caminho do Backup:</translation>
    </message>
    <message>
        <source>Restore Destination:</source>
        <translation>Destino da Restauração:</translation>
    </message>
    <message>
        <source>Destination Path:</source>
        <translation>Caminho de Destino:</translation>
    </message>
    <message>
        <source>Select backup source and restore destination.</source>
        <translation>Selecione a origem do backup e o destino da restauração.</translation>
    </message>
    <message>
        <source>Start Restore</source>
        <translation>Iniciar Restauração</translation>
    </message>
    <message>
        <source>System ISO</source>
        <translation>ISO do Sistema</translation>
    </message>
    <message>
        <source>🗂️ Exclude Big Folders &amp; Files (Home)</source>
        <translation>🗂️ Excluir Pastas e Arquivos Grandes (Home)</translation>
    </message>
    <message>
        <source>Folders &gt;</source>
        <translation>Pastas &gt;</translation>
    </message>
    <message>
        <source>Only show folders larger than this</source>
        <translation>Mostrar apenas pastas maiores que isto</translation>
    </message>
    <message>
        <source>Files &gt;</source>
        <translation>Arquivos &gt;</translation>
    </message>
    <message>
        <source>Only show files larger than this (minimum 10 MB)</source>
        <translation>Mostrar apenas arquivos maiores que isto (mínimo 10 MB)</translation>
    </message>
    <message>
        <source>🔄 Rescan</source>
        <translation>🔄 Reescanear</translation>
    </message>
    <message>
        <source>Checked = contents excluded from the ISO (the folder itself stays, empty).</source>
        <translation>Marcado = conteúdo excluído da ISO (a pasta em si permanece, vazia).</translation>
    </message>
    <message>
        <source>Path</source>
        <translation>Caminho</translation>
    </message>
    <message>
        <source>Size</source>
        <translation>Tamanho</translation>
    </message>
    <message>
        <source>🕵️ Exclude App Configs (~/.config)</source>
        <translation>🕵️ Excluir Configs de Apps (~/.config)</translation>
    </message>
    <message>
        <source>The ISO clones your logins (Discord, Chrome, etc). Check apps to keep their settings/sessions OUT of the ISO — important before sharing it.</source>
        <translation>A ISO clona seus logins (Discord, Chrome, etc). Marque apps para deixar suas configurações/sessões FORA da ISO — importante antes de compartilhá-la.</translation>
    </message>
    <message>
        <source>Application</source>
        <translation>Aplicativo</translation>
    </message>
    <message>
        <source>💾 Save Exclusions</source>
        <translation>💾 Salvar Exclusões</translation>
    </message>
    <message>
        <source>Save the current exclusion checklist to a profile file</source>
        <translation>Salvar a lista de exclusões atual em um arquivo de perfil</translation>
    </message>
    <message>
        <source>📂 Load Exclusions</source>
        <translation>📂 Carregar Exclusões</translation>
    </message>
    <message>
        <source>Load a saved exclusion profile (missing paths are skipped safely)</source>
        <translation>Carregar um perfil de exclusões salvo (caminhos ausentes são ignorados com segurança)</translation>
    </message>
    <message>
        <source>🧹 Clear All</source>
        <translation>🧹 Limpar Tudo</translation>
    </message>
    <message>
        <source>Uncheck every exclusion</source>
        <translation>Desmarcar todas as exclusões</translation>
    </message>
    <message>
        <source>Nothing excluded — the ISO will contain the complete system.</source>
        <translation>Nada excluído — a ISO conterá o sistema completo.</translation>
    </message>
    <message>
        <source>Installation Mode</source>
        <translation>Modo de Instalação</translation>
    </message>
    <message>
        <source>1) Exact Clone (default) — restore the system exactly as it is. Flawless on same/similar hardware.</source>
        <translation type="vanished">1) Clone Exato (padrão) — restaura o sistema exatamente como está. Perfeito em hardware igual/semelhante.</translation>
    </message>
    <message>
        <source>2) Hardware Compatibility Options — first-boot adaptation for DIFFERENT hardware (each optional):</source>
        <translation>2) Opções de Compatibilidade de Hardware — adaptação no primeiro boot para hardware DIFERENTE (cada uma opcional):</translation>
    </message>
    <message>
        <source>    a) Auto-detect network cards — repair bridges &amp; adapter bindings on first boot</source>
        <translation type="vanished">    a) Autodetectar placas de rede — reparar bridges e vínculos de adaptadores no primeiro boot</translation>
    </message>
    <message>
        <source>    b) Auto-detect GPU — NVIDIA: leave untouched; AMD: install drivers + configure X11 &amp; Wayland</source>
        <translation type="vanished">    b) Autodetectar GPU — NVIDIA: não mexer; AMD: instalar drivers + configurar X11 e Wayland</translation>
    </message>
    <message>
        <source>    c) Offer username &amp; password change on first boot</source>
        <translation type="vanished">    c) Oferecer troca de usuário e senha no primeiro boot</translation>
    </message>
    <message>
        <source>    d) Regenerate SSH host keys (new machine identity)</source>
        <translation type="vanished">    d) Regenerar chaves SSH do host (nova identidade de máquina)</translation>
    </message>
    <message>
        <source>Download Mode</source>
        <translation>Modo de Download</translation>
    </message>
    <message>
        <source>Online Mode: Download Required Installation Packages from AUR (4.0GB Download)</source>
        <translation>Modo Online: Baixar os Pacotes de Instalação Necessários do AUR (download de 4.0GB)</translation>
    </message>
    <message>
        <source>Offline Mode: (DO NOT USE... NEEDS TO BE FIXED) Use existing offline-iso-packages.tar.gz previously downloaded (Must be in the root folder of this program)</source>
        <translation>Modo Offline: (NÃO USE... PRECISA SER CORRIGIDO) Usa o offline-iso-packages.tar.gz existente baixado anteriormente (deve estar na pasta raiz deste programa)</translation>
    </message>
    <message>
        <source>Checking offline package availability...</source>
        <translation>Verificando disponibilidade do pacote offline...</translation>
    </message>
    <message>
        <source>📥 Download Offline Package (3.8GB)</source>
        <translation>📥 Baixar Pacote Offline (3.8GB)</translation>
    </message>
    <message>
        <source>🔄 Check Availability</source>
        <translation>🔄 Verificar Disponibilidade</translation>
    </message>
    <message>
        <source>System Clone Information</source>
        <translation>Informações do Clone do Sistema</translation>
    </message>
    <message>
        <source>This tool creates a bootable ISO that contains an EXACT clone of your current system. The ISO will include:</source>
        <translation>Esta ferramenta cria uma ISO bootável contendo um clone EXATO do seu sistema atual. A ISO incluirá:</translation>
    </message>
    <message>
        <source>After the ISO is ready:

1. Burn the ISO to a USB drive using Balena Etcher, Rufus, or Ventoy.

   ⚠ WARNING (Ventoy): Eject the USB drive from your system before unplugging it. Removing it without ejecting can corrupt the ISO and make it unbootable.

2. Boot your target machine from the USB (live environment).

3. The XETAL ENGINE installer launches AUTOMATICALLY — pick the target drive with the arrow keys (or mouse) and confirm. No commands needed.

   (If you ever exit the installer, restart it with: /xetal.sh)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
PRIVACY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
• This ISO contains a full clone of your system, including personal data and saved logins (e.g. browser, Discord). Keep it private.
• Before sharing an ISO, use the exclusion panels above: drop big folders you don't need, and exclude app configs (Discord, browsers...) so your logins don't ship with the ISO.</source>
        <translation>Quando a ISO estiver pronta:

1. Grave a ISO em um pendrive usando Balena Etcher, Rufus ou Ventoy.

   ⚠ AVISO (Ventoy): Ejete o pendrive do sistema antes de removê-lo. Remover sem ejetar pode corromper a ISO e torná-la não-bootável.

2. Inicialize a máquina de destino pelo USB (ambiente live).

3. O instalador XETAL ENGINE inicia AUTOMATICAMENTE — escolha o disco de destino com as setas (ou mouse) e confirme. Nenhum comando é necessário.

   (Se você sair do instalador, reinicie-o com: /xetal.sh)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
PRIVACIDADE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
• Esta ISO contém um clone completo do seu sistema, incluindo dados pessoais e logins salvos (ex.: navegador, Discord). Mantenha-a privada.
• Antes de compartilhar uma ISO, use os painéis de exclusão acima: remova pastas grandes desnecessárias e exclua configs de apps (Discord, navegadores...) para que seus logins não sejam incluídos.</translation>
    </message>
    <message>
        <source>Ready to create system clone ISO. Output will be saved to /home/user/iso/xiso/output/</source>
        <translation>Pronto para criar a ISO de clone do sistema. A saída será salva em /home/user/iso/xiso/output/</translation>
    </message>
    <message>
        <source>🚀 Create System Clone ISO</source>
        <translation>🚀 Criar ISO de Clone do Sistema</translation>
    </message>
    <message>
        <source>📁 Browse</source>
        <translation>📁 Procurar</translation>
    </message>
    <message>
        <source>Browse to ISO output folder using Dolphin</source>
        <translation>Abrir a pasta de saída da ISO no Dolphin</translation>
    </message>
    <message>
        <source>Network</source>
        <translation>Rede</translation>
    </message>
    <message>
        <source>Network Information</source>
        <translation>Informações de Rede</translation>
    </message>
    <message>
        <source>IP Addresses</source>
        <translation>Endereços IP</translation>
    </message>
    <message>
        <source>Interface</source>
        <translation>Interface</translation>
    </message>
    <message>
        <source>IP Address</source>
        <translation>Endereço IP</translation>
    </message>
    <message>
        <source>Netmask</source>
        <translation>Máscara de Rede</translation>
    </message>
    <message>
        <source>Network Statistics</source>
        <translation>Estatísticas de Rede</translation>
    </message>
    <message>
        <source>Local IP:</source>
        <translation>IP Local:</translation>
    </message>
    <message>
        <source>Public IP:</source>
        <translation>IP Público:</translation>
    </message>
    <message>
        <source>Hostname:</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>DNS Servers:</source>
        <translation>Servidores DNS:</translation>
    </message>
    <message>
        <source>Detailed Interface Statistics</source>
        <translation>Estatísticas Detalhadas da Interface</translation>
    </message>
    <message>
        <source>RX Bytes</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>TX Bytes</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>RX Packets</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>TX Packets</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>RX Errors</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>TX Errors</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Speed</source>
        <translation>Velocidade</translation>
    </message>
    <message>
        <source>Interface Configuration</source>
        <translation>Configuração de Interface</translation>
    </message>
    <message>
        <source>Configuration</source>
        <translation>Configuração</translation>
    </message>
    <message>
        <source>Gateway</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>⚙️ Configure Interface</source>
        <translation>⚙️ Configurar Interface</translation>
    </message>
    <message>
        <source>⬆️ Bring Up</source>
        <translation>⬆️ Ativar</translation>
    </message>
    <message>
        <source>⬇️ Bring Down</source>
        <translation>⬇️ Desativar</translation>
    </message>
    <message>
        <source>Bridge Configuration</source>
        <translation>Configuração de Bridge</translation>
    </message>
    <message>
        <source>Bridge Name</source>
        <translation>Nome da Bridge</translation>
    </message>
    <message>
        <source>Interfaces</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>➕ Create Bridge</source>
        <translation>➕ Criar Bridge</translation>
    </message>
    <message>
        <source>🗑️ Delete Bridge</source>
        <translation>🗑️ Excluir Bridge</translation>
    </message>
    <message>
        <source>🔧 Fix Bridge (New Hardware)</source>
        <translation>🔧 Corrigir Bridge (Hardware Novo)</translation>
    </message>
    <message>
        <source>Re-attach the bridge to whatever network device this machine has. Use after installing a system image on different hardware.</source>
        <translation>Reanexa a bridge ao dispositivo de rede desta máquina. Use após instalar uma imagem do sistema em hardware diferente.</translation>
    </message>
    <message>
        <source>Libvirt Networks</source>
        <translation>Redes Libvirt</translation>
    </message>
    <message>
        <source>Network Name</source>
        <translation>Nome da Rede</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Tipo</translation>
    </message>
    <message>
        <source>State</source>
        <translation>Estado</translation>
    </message>
    <message>
        <source>Autostart</source>
        <translation>Início Automático</translation>
    </message>
    <message>
        <source>▶️ Start</source>
        <translation>▶️ Iniciar</translation>
    </message>
    <message>
        <source>⏸️ Stop</source>
        <translation>⏸️ Parar</translation>
    </message>
    <message>
        <source>🔄 Restart</source>
        <translation>🔄 Reiniciar</translation>
    </message>
    <message>
        <source>Wi-Fi</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>📡 Scan Networks</source>
        <translation>📡 Buscar Redes</translation>
    </message>
    <message>
        <source>🔗 Connect</source>
        <translation>🔗 Conectar</translation>
    </message>
    <message>
        <source>❌ Disconnect</source>
        <translation>❌ Desconectar</translation>
    </message>
    <message>
        <source>📶 Wi-Fi On/Off</source>
        <translation>📶 Ligar/Desligar Wi-Fi</translation>
    </message>
    <message>
        <source>Click Scan to list nearby networks.</source>
        <translation>Clique em Buscar para listar redes próximas.</translation>
    </message>
    <message>
        <source>Saved Connections</source>
        <translation>Conexões Salvas</translation>
    </message>
    <message>
        <source>⬆️ Activate</source>
        <translation>⬆️ Ativar</translation>
    </message>
    <message>
        <source>⬇️ Deactivate</source>
        <translation>⬇️ Desativar</translation>
    </message>
    <message>
        <source>♻️ Toggle Autoconnect</source>
        <translation>♻️ Alternar Autoconexão</translation>
    </message>
    <message>
        <source>🗑️ Delete</source>
        <translation>🗑️ Excluir</translation>
    </message>
    <message>
        <source>🔁 Restart NetworkManager</source>
        <translation>🔁 Reiniciar NetworkManager</translation>
    </message>
    <message>
        <source>Open Ports</source>
        <translation>Portas Abertas</translation>
    </message>
    <message>
        <source>Programs listening on your network ports.</source>
        <translation>Programas escutando nas portas de rede.</translation>
    </message>
    <message>
        <source>PKG Install</source>
        <translation>Instalar PKG</translation>
    </message>
    <message>
        <source>🗑️ Clear Pacman Cache</source>
        <translation>🗑️ Limpar Cache do Pacman</translation>
    </message>
    <message>
        <source>Clear pacman package cache</source>
        <translation>Limpar o cache de pacotes do pacman</translation>
    </message>
    <message>
        <source>🔄 Refresh All</source>
        <translation>🔄 Atualizar Tudo</translation>
    </message>
    <message>
        <source>📦 Install .deb...</source>
        <translation>📦 Instalar .deb...</translation>
    </message>
    <message>
        <source>Convert a Debian package with debtap and install it as a native pacman package</source>
        <translation>Converte um pacote Debian com o debtap e instala como pacote nativo do pacman</translation>
    </message>
    <message>
        <source>Yay: Not Installed</source>
        <translation>Yay: Não Instalado</translation>
    </message>
    <message>
        <source>Install Yay</source>
        <translation>Instalar Yay</translation>
    </message>
    <message>
        <source>Reinstall Yay</source>
        <translation>Reinstalar Yay</translation>
    </message>
    <message>
        <source>Uninstall Yay</source>
        <translation>Desinstalar Yay</translation>
    </message>
    <message>
        <source>Paru: Not Installed</source>
        <translation>Paru: Não Instalado</translation>
    </message>
    <message>
        <source>Install Paru</source>
        <translation>Instalar Paru</translation>
    </message>
    <message>
        <source>Reinstall Paru</source>
        <translation>Reinstalar Paru</translation>
    </message>
    <message>
        <source>Uninstall Paru</source>
        <translation>Desinstalar Paru</translation>
    </message>
    <message>
        <source>📦 Pacman</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Installed Packages</source>
        <translation>Pacotes Instalados</translation>
    </message>
    <message>
        <source>Search installed packages...</source>
        <translation>Pesquisar pacotes instalados...</translation>
    </message>
    <message>
        <source>Uninstall Selected</source>
        <translation>Desinstalar Selecionados</translation>
    </message>
    <message>
        <source>Reinstall Selected</source>
        <translation>Reinstalar Selecionados</translation>
    </message>
    <message>
        <source>AUR Search</source>
        <translation>Busca no AUR</translation>
    </message>
    <message>
        <source>Search AUR packages...</source>
        <translation>Pesquisar pacotes do AUR...</translation>
    </message>
    <message>
        <source>Search mode</source>
        <translation>Modo de busca</translation>
    </message>
    <message>
        <source>Package Name</source>
        <translation>Nome do Pacote</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Descrição</translation>
    </message>
    <message>
        <source>Both (Name or Desc)</source>
        <translation>Ambos (Nome ou Desc)</translation>
    </message>
    <message>
        <source>Search type</source>
        <translation>Tipo de busca</translation>
    </message>
    <message>
        <source>Exact</source>
        <translation>Exato</translation>
    </message>
    <message>
        <source>Contains</source>
        <translation>Contém</translation>
    </message>
    <message>
        <source>🔍 Search</source>
        <translation>🔍 Buscar</translation>
    </message>
    <message>
        <source>Install Selected</source>
        <translation>Instalar Selecionados</translation>
    </message>
    <message>
        <source>🔧 Yay</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Search packages...</source>
        <translation>Pesquisar pacotes...</translation>
    </message>
    <message>
        <source>⚡ Paru</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>🔍 Check for Updates</source>
        <translation>🔍 Verificar Atualizações</translation>
    </message>
    <message>
        <source>⬆️ Upgrade System (pacman -Syu)</source>
        <translation>⬆️ Atualizar Sistema (pacman -Syu)</translation>
    </message>
    <message>
        <source>⬆️ Upgrade incl. AUR</source>
        <translation>⬆️ Atualizar incl. AUR</translation>
    </message>
    <message>
        <source>Click Check for Updates.</source>
        <translation>Clique em Verificar Atualizações.</translation>
    </message>
    <message>
        <source>📰 Arch Linux News (read before big upgrades!):</source>
        <translation>📰 Notícias do Arch Linux (leia antes de grandes atualizações!):</translation>
    </message>
    <message>
        <source>Package operation output will appear here...</source>
        <translation>A saída das operações de pacotes aparecerá aqui...</translation>
    </message>
    <message>
        <source>PKG Uninstall</source>
        <translation>Desinstalar PKG</translation>
    </message>
    <message>
        <source>Search installed software...</source>
        <translation>Pesquisar software instalado...</translation>
    </message>
    <message>
        <source>Filter by package source</source>
        <translation>Filtrar por origem do pacote</translation>
    </message>
    <message>
        <source>All Sources</source>
        <translation>Todas as Origens</translation>
    </message>
    <message>
        <source>Official Repos</source>
        <translation>Repos Oficiais</translation>
    </message>
    <message>
        <source>AUR / Foreign</source>
        <translation>AUR / Externo</translation>
    </message>
    <message>
        <source>Click Refresh to load installed software.</source>
        <translation>Clique em Atualizar para carregar o software instalado.</translation>
    </message>
    <message>
        <source>Remove with:</source>
        <translation>Remover com:</translation>
    </message>
    <message>
        <source>Tool used to remove the selected packages. Auto picks pacman.</source>
        <translation>Ferramenta usada para remover os pacotes selecionados. Auto escolhe o pacman.</translation>
    </message>
    <message>
        <source>Auto</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Pacman</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Yay</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Paru</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Also remove unneeded dependencies (-Rns)</source>
        <translation>Também remover dependências desnecessárias (-Rns)</translation>
    </message>
    <message>
        <source>🗑️ Uninstall Selected</source>
        <translation>🗑️ Desinstalar Selecionados</translation>
    </message>
    <message>
        <source>♻️ Clean Orphans</source>
        <translation>♻️ Limpar Órfãos</translation>
    </message>
    <message>
        <source>🧹 Clean Pacman Cache</source>
        <translation>🧹 Limpar Cache do Pacman</translation>
    </message>
    <message>
        <source>Selected: 0 packages</source>
        <translation>Selecionados: 0 pacotes</translation>
    </message>
    <message>
        <source>Theme</source>
        <translation>Tema</translation>
    </message>
    <message>
        <source>🎨 Preset Themes</source>
        <translation>🎨 Temas Predefinidos</translation>
    </message>
    <message>
        <source>Theme:</source>
        <translation>Tema:</translation>
    </message>
    <message>
        <source>System Default (XETAL Green)</source>
        <translation>Padrão do Sistema (XETAL Verde)</translation>
    </message>
    <message>
        <source>XETAL Dark</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>XETAL Red</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Midnight Blue</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Matrix</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Solar Light</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Purple Haze</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Custom</source>
        <translation>Personalizado</translation>
    </message>
    <message>
        <source>🖌️ Custom Colors (changes apply instantly and are remembered)</source>
        <translation>🖌️ Cores Personalizadas (mudanças aplicam na hora e são lembradas)</translation>
    </message>
    <message>
        <source>Accent color (stripes, hovers, highlights):</source>
        <translation>Cor de destaque (listras, hovers, seleções):</translation>
    </message>
    <message>
        <source>Pick...</source>
        <translation>Escolher...</translation>
    </message>
    <message>
        <source>Background color (empty = system theme):</source>
        <translation>Cor de fundo (vazio = tema do sistema):</translation>
    </message>
    <message>
        <source>System (default)</source>
        <translation>Sistema (padrão)</translation>
    </message>
    <message>
        <source>Text color (empty = auto contrast):</source>
        <translation>Cor do texto (vazio = contraste automático):</translation>
    </message>
    <message>
        <source>↩️ Reset to System Default</source>
        <translation>↩️ Redefinir para o Padrão do Sistema</translation>
    </message>
    <message>
        <source>Tip: the accent color drives the stripes on every card, button hovers, table headers, selections, and progress bars. Background + text colors re-skin the whole app — or leave them on System to follow your desktop theme.</source>
        <translation>Dica: a cor de destaque controla as listras de cada cartão, hovers de botões, cabeçalhos de tabelas, seleções e barras de progresso. Cores de fundo + texto re-estilizam o app todo — ou deixe em Sistema para seguir o tema do desktop.</translation>
    </message>
    <message>
        <source>File</source>
        <translation>Arquivo</translation>
    </message>
    <message>
        <source>Preferences</source>
        <translation>Preferências</translation>
    </message>
    <message>
        <source>Help</source>
        <translation>Ajuda</translation>
    </message>
    <message>
        <source>Exit</source>
        <translation>Sair</translation>
    </message>
    <message>
        <source>About</source>
        <translation>Sobre</translation>
    </message>
    <message>
        <source>Show/Hide Tabs</source>
        <translation>Mostrar/Ocultar Abas</translation>
    </message>
    <message>
        <source>Language Changed</source>
        <translation>Idioma Alterado</translation>
    </message>
    <message>
        <source>The new language takes effect after a restart.

Restart CachyOsTools now?</source>
        <translation>O novo idioma entra em vigor após reiniciar.

Reiniciar o CachyOsTools agora?</translation>
    </message>
    <message>
        <source>Kernel &lt;b&gt;%1&lt;/b&gt; &amp;nbsp;·&amp;nbsp; up %2</source>
        <translation>Kernel &lt;b&gt;%1&lt;/b&gt; &amp;nbsp;·&amp;nbsp; ativo há %2</translation>
    </message>
    <message>
        <source>%1 used of %2</source>
        <translation>%1 usados de %2</translation>
    </message>
    <message>
        <source>No data</source>
        <translation>Sem dados</translation>
    </message>
    <message>
        <source>All services healthy</source>
        <translation>Todos os serviços saudáveis</translation>
    </message>
    <message>
        <source>%1 services running</source>
        <translation>%1 serviços em execução</translation>
    </message>
    <message>
        <source>failed service(s)</source>
        <translation>serviço(s) com falha</translation>
    </message>
    <message>
        <source>System is up to date</source>
        <translation>Sistema atualizado</translation>
    </message>
    <message>
        <source> +%1 more</source>
        <translation> +%1 outros</translation>
    </message>
    <message>
        <source>update(s) pending</source>
        <translation>atualização(ões) pendente(s)</translation>
    </message>
    <message>
        <source>&lt;p style='color:#888;'&gt;Install &lt;b&gt;pacman-contrib&lt;/b&gt; to enable update checks.&lt;/p&gt;</source>
        <translation>&lt;p style='color:#888;'&gt;Instale &lt;b&gt;pacman-contrib&lt;/b&gt; para habilitar a verificação de atualizações.&lt;/p&gt;</translation>
    </message>
    <message>
        <source>No safety ISO yet</source>
        <translation>Nenhuma ISO de segurança ainda</translation>
    </message>
    <message>
        <source>Create one in the System ISO tab —&lt;br&gt;your insurance against drive failure.</source>
        <translation>Crie uma na aba ISO do Sistema —&lt;br&gt;seu seguro contra falhas de disco.</translation>
    </message>
    <message>
        <source>today</source>
        <translation>hoje</translation>
    </message>
    <message>
        <source>yesterday</source>
        <translation>ontem</translation>
    </message>
    <message>
        <source>%1 days ago</source>
        <translation>há %1 dias</translation>
    </message>
    <message>
        <source>last safety ISO</source>
        <translation>última ISO de segurança</translation>
    </message>
    <message>
        <source>&lt;p style='color:#888;'&gt;🔒 Health checks need a sudo session.&lt;br&gt;&lt;br&gt;Use &lt;b&gt;Load Labels&lt;/b&gt; or &lt;b&gt;Health Check&lt;/b&gt; in the Drives tab once, then Refresh here.&lt;/p&gt;</source>
        <translation>&lt;p style='color:#888;'&gt;🔒 As verificações de saúde precisam de uma sessão sudo.&lt;br&gt;&lt;br&gt;Use &lt;b&gt;Carregar Rótulos&lt;/b&gt; ou &lt;b&gt;Verificação de Saúde&lt;/b&gt; na aba Discos uma vez e clique em Atualizar aqui.&lt;/p&gt;</translation>
    </message>
    <message>
        <source>&lt;p style='color:#888;'&gt;No SMART data&lt;br&gt;(is smartmontools installed?)&lt;/p&gt;</source>
        <translation>&lt;p style='color:#888;'&gt;Sem dados SMART&lt;br&gt;(smartmontools está instalado?)&lt;/p&gt;</translation>
    </message>
    <message>
        <source>⬆️ UPDATE — %1 new commit(s) available!</source>
        <translation>⬆️ ATUALIZAR — %1 novo(s) commit(s) disponível(is)!</translation>
    </message>
    <message>
        <source>Latest change: %1
Click to pull, rebuild and restart.</source>
        <translation>Última mudança: %1
Clique para baixar, recompilar e reiniciar.</translation>
    </message>
    <message>
        <source>Click to pull, rebuild and restart.</source>
        <translation>Clique para baixar, recompilar e reiniciar.</translation>
    </message>
    <message>
        <source>Not a Git Install</source>
        <translation>Instalação sem Git</translation>
    </message>
    <message>
        <source>The running app was not started from a git checkout, so it cannot self-update.</source>
        <translation>O app em execução não foi iniciado de um checkout git, então não pode se autoatualizar.</translation>
    </message>
    <message>
        <source>

⚠ The repo has local uncommitted changes — the pull will refuse to overwrite them. Commit or stash first if the update fails.</source>
        <translation>

⚠ O repositório tem mudanças locais não commitadas — o pull se recusará a sobrescrevê-las. Faça commit ou stash primeiro se a atualização falhar.</translation>
    </message>
    <message>
        <source>Update CachyOsTools</source>
        <translation>Atualizar CachyOsTools</translation>
    </message>
    <message>
        <source>This will, in a visible terminal:

1. Pull the newest version into:
    %1
2. Rebuild the app
3. Offer to restart when done%2

Continue?</source>
        <translation>Isto fará, em um terminal visível:

1. Baixar a versão mais nova em:
    %1
2. Recompilar o app
3. Oferecer reinício ao terminar%2

Continuar?</translation>
    </message>
    <message>
        <source>Restart CachyOsTools?</source>
        <translation>Reiniciar o CachyOsTools?</translation>
    </message>
    <message>
        <source>If the update succeeded, a restart loads the new version.

Restart now?</source>
        <translation>Se a atualização deu certo, reiniciar carrega a nova versão.

Reiniciar agora?</translation>
    </message>
    <message>
        <source>Terminal Not Found</source>
        <translation>Terminal Não Encontrado</translation>
    </message>
    <message>
        <source>Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty</source>
        <translation>Não foi possível encontrar um emulador de terminal adequado. Instale um destes: konsole, gnome-terminal, xterm, alacritty ou kitty</translation>
    </message>
    <message>
        <source>Arch OS Tools - Linux System Manager</source>
        <translation>Arch OS Tools - Gerenciador de Sistema Linux</translation>
    </message>
    <message>
        <source>Select destination to begin system backup.</source>
        <translation>Selecione o destino para iniciar o backup do sistema.</translation>
    </message>
    <message>
        <source>Online mode selected - packages will be downloaded during ISO creation</source>
        <translation>Modo online selecionado - os pacotes serão baixados durante a criação da ISO</translation>
    </message>
    <message>
        <source>GPU Type</source>
        <translation>Tipo de GPU</translation>
    </message>
    <message>
        <source>Which GPU vendor do you want to blacklist?

Yes = NVIDIA
No = AMD</source>
        <translation>Qual fabricante de GPU você quer colocar na blacklist?

Sim = NVIDIA
Não = AMD</translation>
    </message>
    <message>
        <source>No Blacklist Found</source>
        <translation>Nenhuma Blacklist Encontrada</translation>
    </message>
    <message>
        <source>No GPU blacklist configuration found. Create one first using the Edit button.</source>
        <translation>Nenhuma configuração de blacklist de GPU encontrada. Crie uma primeiro usando o botão Editar.</translation>
    </message>
    <message>
        <source>Device Name</source>
        <translation>Nome do Dispositivo</translation>
    </message>
    <message>
        <source>Device Path</source>
        <translation>Caminho do Dispositivo</translation>
    </message>
    <message>
        <source>Label</source>
        <translation>Rótulo</translation>
    </message>
    <message>
        <source>Mount Point</source>
        <translation>Ponto de Montagem</translation>
    </message>
    <message>
        <source>Disk ID</source>
        <translation>ID do Disco</translation>
    </message>
    <message>
        <source>No Drive Selected</source>
        <translation>Nenhum Disco Selecionado</translation>
    </message>
    <message>
        <source>Please select a drive to mount.</source>
        <translation>Selecione um disco para montar.</translation>
    </message>
    <message>
        <source>Please select a drive to unmount.</source>
        <translation>Selecione um disco para desmontar.</translation>
    </message>
    <message>
        <source>Not mounted</source>
        <translation>Não montado</translation>
    </message>
    <message>
        <source>✓ Mounted</source>
        <translation>✓ Montado</translation>
    </message>
    <message>
        <source>✗ Unmounted</source>
        <translation>✗ Desmontado</translation>
    </message>
    <message>
        <source>Please select a drive to eject.</source>
        <translation>Selecione um disco para ejetar.</translation>
    </message>
    <message>
        <source>Invalid Selection</source>
        <translation>Seleção Inválida</translation>
    </message>
    <message>
        <source>Eject can only be used on disk devices, not partitions.</source>
        <translation>Ejetar só pode ser usado em discos, não em partições.</translation>
    </message>
    <message>
        <source>Please select a drive or partition to format.</source>
        <translation>Selecione um disco ou partição para formatar.</translation>
    </message>
    <message>
        <source>Drive Mounted</source>
        <translation>Disco Montado</translation>
    </message>
    <message>
        <source>Please unmount the drive before formatting.</source>
        <translation>Desmonte o disco antes de formatar.</translation>
    </message>
    <message>
        <source>Format Drive</source>
        <translation>Formatar Disco</translation>
    </message>
    <message>
        <source>Filesystem:</source>
        <translation>Sistema de arquivos:</translation>
    </message>
    <message>
        <source>Format</source>
        <translation>Formatar</translation>
    </message>
    <message>
        <source>Local accounts and their group memberships (wheel = sudo).</source>
        <translation>Contas locais e seus grupos (wheel = sudo).</translation>
    </message>
    <message>
        <source>➕ Add User</source>
        <translation>➕ Adicionar Usuário</translation>
    </message>
    <message>
        <source>🗑️ Delete User</source>
        <translation>🗑️ Excluir Usuário</translation>
    </message>
    <message>
        <source>🔑 Change Password</source>
        <translation>🔑 Alterar Senha</translation>
    </message>
    <message>
        <source>🐚 Change Shell</source>
        <translation>🐚 Alterar Shell</translation>
    </message>
    <message>
        <source>👥 Edit Groups</source>
        <translation>👥 Editar Grupos</translation>
    </message>
    <message>
        <source>🖼️ Change Icon</source>
        <translation>🖼️ Alterar Ícone</translation>
    </message>
    <message>
        <source>User</source>
        <translation>Usuário</translation>
    </message>
    <message>
        <source>UID</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Home</source>
        <translation>Home</translation>
    </message>
    <message>
        <source>Groups</source>
        <translation>Grupos</translation>
    </message>
    <message>
        <source>Add User</source>
        <translation>Adicionar Usuário</translation>
    </message>
    <message>
        <source>Username (lowercase, no spaces):</source>
        <translation>Nome de usuário (minúsculas, sem espaços):</translation>
    </message>
    <message>
        <source>Invalid Name</source>
        <translation>Nome Inválido</translation>
    </message>
    <message>
        <source>Use lowercase letters, digits, - and _ only.</source>
        <translation>Use apenas letras minúsculas, dígitos, - e _.</translation>
    </message>
    <message>
        <source>Sudo Access</source>
        <translation>Acesso Sudo</translation>
    </message>
    <message>
        <source>Should '%1' be an administrator (wheel group)?</source>
        <translation>'%1' deve ser administrador (grupo wheel)?</translation>
    </message>
    <message>
        <source>Select a user first.</source>
        <translation>Selecione um usuário primeiro.</translation>
    </message>
    <message>
        <source>You cannot delete the account you are logged in with.</source>
        <translation>Você não pode excluir a conta com a qual está logado.</translation>
    </message>
    <message>
        <source>Deleting root would be a very short story.</source>
        <translation>Excluir o root seria uma história bem curta.</translation>
    </message>
    <message>
        <source>Delete User</source>
        <translation>Excluir Usuário</translation>
    </message>
    <message>
        <source>Delete '%1' AND its home directory? This cannot be undone.</source>
        <translation>Excluir '%1' E seu diretório home? Isso não pode ser desfeito.</translation>
    </message>
    <message>
        <source>Change Shell</source>
        <translation>Alterar Shell</translation>
    </message>
    <message>
        <source>Login shell for %1:</source>
        <translation>Shell de login para %1:</translation>
    </message>
    <message>
        <source>Groups for %1</source>
        <translation>Grupos de %1</translation>
    </message>
    <message>
        <source>Checked = member. Important: wheel (sudo), libvirt, docker, input, video.</source>
        <translation>Marcado = membro. Importantes: wheel (sudo), libvirt, docker, input, video.</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Cancelar</translation>
    </message>
    <message>
        <source>Choose Avatar for %1</source>
        <translation>Escolher Avatar para %1</translation>
    </message>
    <message>
        <source>Images (*.png *.jpg *.jpeg *.bmp *.webp)</source>
        <translation>Imagens (*.png *.jpg *.jpeg *.bmp *.webp)</translation>
    </message>
    <message>
        <source>Invalid Image</source>
        <translation>Imagem Inválida</translation>
    </message>
    <message>
        <source>Could not read that image file.</source>
        <translation>Não foi possível ler esse arquivo de imagem.</translation>
    </message>
    <message>
        <source>Failed</source>
        <translation>Falhou</translation>
    </message>
    <message>
        <source>Could not prepare the avatar image.</source>
        <translation>Não foi possível preparar a imagem do avatar.</translation>
    </message>
    <message>
        <source>Please select a drive to view SMART information.</source>
        <translation>Selecione um disco para ver as informações SMART.</translation>
    </message>
    <message>
        <source>SMART information is only available for disk devices, not partitions.</source>
        <translation>Informações SMART só estão disponíveis para discos, não partições.</translation>
    </message>
    <message>
        <source>SMART Not Available</source>
        <translation>SMART Não Disponível</translation>
    </message>
    <message>
        <source>smartmontools is not installed.

Install it with: sudo pacman -S smartmontools</source>
        <translation>smartmontools não está instalado.

Instale com: sudo pacman -S smartmontools</translation>
    </message>
    <message>
        <source>Retrieving SMART information...</source>
        <translation>Obtendo informações SMART...</translation>
    </message>
    <message>
        <source>Every PCI and USB device, the kernel driver it uses, and its modules.</source>
        <translation>Todos os dispositivos PCI e USB, o driver do kernel em uso e seus módulos.</translation>
    </message>
    <message>
        <source>ℹ️ Module Info</source>
        <translation>ℹ️ Info do Módulo</translation>
    </message>
    <message>
        <source>▶️ Load Module</source>
        <translation>▶️ Carregar Módulo</translation>
    </message>
    <message>
        <source>⏏️ Unload Module</source>
        <translation>⏏️ Descarregar Módulo</translation>
    </message>
    <message>
        <source>🚫 Blacklist Module</source>
        <translation>🚫 Blacklist do Módulo</translation>
    </message>
    <message>
        <source>Driver in use</source>
        <translation>Driver em uso</translation>
    </message>
    <message>
        <source>Kernel modules</source>
        <translation>Módulos do kernel</translation>
    </message>
    <message>
        <source>No Module</source>
        <translation>Sem Módulo</translation>
    </message>
    <message>
        <source>Select a device that has a kernel module.</source>
        <translation>Selecione um dispositivo que tenha um módulo do kernel.</translation>
    </message>
    <message>
        <source>No modinfo available for '%1' (may be built into the kernel).</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Module Info — %1</source>
        <translation>Info do Módulo — %1</translation>
    </message>
    <message>
        <source>Close</source>
        <translation>Fechar</translation>
    </message>
    <message>
        <source>Unload Module</source>
        <translation>Descarregar Módulo</translation>
    </message>
    <message>
        <source>Unloading '%1' can take away your display, network or input if it is in use.

Continue?</source>
        <translation>Descarregar '%1' pode derrubar sua tela, rede ou entrada se estiver em uso.

Continuar?</translation>
    </message>
    <message>
        <source>Blacklist Module</source>
        <translation>Blacklist do Módulo</translation>
    </message>
    <message>
        <source>This writes 'blacklist %1' to /etc/modprobe.d/cachyostools-blacklist.conf so it never loads at boot.

Blacklisting your GPU, disk or network driver will break the system. Continue?</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>USB Devices</source>
        <translation>Dispositivos USB</translation>
    </message>
    <message>
        <source>SMART Information:</source>
        <translation>Informações SMART:</translation>
    </message>
    <message>
        <source>Error retrieving SMART information:</source>
        <translation>Erro ao obter informações SMART:</translation>
    </message>
    <message>
        <source>NTFS Hibernation Warning</source>
        <translation>Aviso de Hibernação NTFS</translation>
    </message>
    <message>
        <source>If a Windows hibernation file is present, it will be deleted to allow write access. Unsaved Windows session data will be lost.</source>
        <translation>Se houver um arquivo de hibernação do Windows, ele será excluído para permitir escrita. Dados de sessão do Windows não salvos serão perdidos.</translation>
    </message>
    <message>
        <source>If the drive is still not writable after mounting, please check for Windows Fast Startup, hibernation, or filesystem errors.</source>
        <translation>Se o disco ainda não permitir escrita após montar, verifique o Fast Startup do Windows, hibernação ou erros no sistema de arquivos.</translation>
    </message>
    <message>
        <source>Please select a drive to take ownership of.</source>
        <translation>Selecione um disco para assumir a propriedade.</translation>
    </message>
    <message>
        <source>Not Mounted</source>
        <translation>Não Montado</translation>
    </message>
    <message>
        <source>The selected drive is not mounted.</source>
        <translation>O disco selecionado não está montado.</translation>
    </message>
    <message>
        <source>This will recursively change the owner and permissions of all files on the drive to your user (chown -R and chmod -R 777). Continue?</source>
        <translation>Isto mudará recursivamente o dono e as permissões de todos os arquivos do disco para o seu usuário (chown -R e chmod -R 777). Continuar?</translation>
    </message>
    <message>
        <source>Please select a valid destination. The source is automatically set to '/' for system backup.</source>
        <translation>Selecione um destino válido. A origem é definida automaticamente como '/' para backup do sistema.</translation>
    </message>
    <message>
        <source>Could not launch a terminal emulator. Please install konsole, gnome-terminal, xterm, alacritty, or kitty.</source>
        <translation>Não foi possível abrir um terminal. Instale konsole, gnome-terminal, xterm, alacritty ou kitty.</translation>
    </message>
    <message>
        <source>Backup completed successfully.</source>
        <translation>Backup concluído com sucesso.</translation>
    </message>
    <message>
        <source>Backup failed. Please check destination and permissions.</source>
        <translation>Backup falhou. Verifique o destino e as permissões.</translation>
    </message>
    <message>
        <source>Firewall Backup</source>
        <translation>Backup do Firewall</translation>
    </message>
    <message>
        <source>Firewall rules are stored in:
- firewalld: /etc/firewalld/
- ufw: /etc/ufw/

Backup these directories manually if needed.</source>
        <translation>As regras de firewall ficam em:
- firewalld: /etc/firewalld/
- ufw: /etc/ufw/

Faça backup desses diretórios manualmente se necessário.</translation>
    </message>
    <message>
        <source>Please select both source backup and destination.</source>
        <translation>Selecione o backup de origem e o destino.</translation>
    </message>
    <message>
        <source>System Restore</source>
        <translation>Restauração do Sistema</translation>
    </message>
    <message>
        <source>This will restore your system backup to the root filesystem.

⚠️  WARNING: This will overwrite the current system!

Make sure you:
1. Have a working live environment ready
2. Have backed up any important data
3. Are running this from a live environment

Continue with system restore?</source>
        <translation>Isto restaurará o backup do sistema no sistema de arquivos raiz.

⚠️  AVISO: Isto sobrescreverá o sistema atual!

Certifique-se de que você:
1. Tem um ambiente live funcional pronto
2. Fez backup de dados importantes
3. Está executando isto de um ambiente live

Continuar com a restauração do sistema?</translation>
    </message>
    <message>
        <source>Please select valid source and destination (must be different drives, not /mnt or /media, and not subfolders of each other).</source>
        <translation>Selecione origem e destino válidos (devem ser discos diferentes, não /mnt ou /media, e não subpastas um do outro).</translation>
    </message>
    <message>
        <source>Error</source>
        <translation>Erro</translation>
    </message>
    <message>
        <source>Copied to clipboard</source>
        <translation>Copiado para a área de transferência</translation>
    </message>
    <message>
        <source>Could not save file.</source>
        <translation>Não foi possível salvar o arquivo.</translation>
    </message>
    <message>
        <source>Edit</source>
        <translation>Editar</translation>
    </message>
    <message>
        <source>File Path</source>
        <translation>Caminho do Arquivo</translation>
    </message>
    <message>
        <source>Copy to Clipboard</source>
        <translation>Copiar para Área de Transferência</translation>
    </message>
    <message>
        <source>TRIM Configuration</source>
        <translation>Configuração de TRIM</translation>
    </message>
    <message>
        <source>TRIM is managed via systemd timer, not a config file.

To enable/disable TRIM, use the 'Apply' button or run:
sudo systemctl enable fstrim.timer
sudo systemctl start fstrim.timer

TRIM can also be enabled via mount options in /etc/fstab
by adding 'discard' option, but this is not recommended
as it can impact performance.</source>
        <translation>O TRIM é gerenciado por um timer do systemd, não por arquivo de config.

Para ativar/desativar o TRIM, use o botão 'Aplicar' ou execute:
sudo systemctl enable fstrim.timer
sudo systemctl start fstrim.timer

O TRIM também pode ser ativado via opções de montagem no /etc/fstab
adicionando a opção 'discard', mas isso não é recomendado
pois pode impactar o desempenho.</translation>
    </message>
    <message>
        <source>Apply ZRAM</source>
        <translation>Aplicar ZRAM</translation>
    </message>
    <message>
        <source>This will enable ZRAM (compressed RAM swap).

A config file will be created at /etc/systemd/zram-generator.conf
and the service will be started.

Continue?</source>
        <translation>Isto ativará o ZRAM (swap de RAM comprimida).

Um arquivo de config será criado em /etc/systemd/zram-generator.conf
e o serviço será iniciado.

Continuar?</translation>
    </message>
    <message>
        <source>Could not find a suitable terminal emulator.</source>
        <translation>Não foi possível encontrar um emulador de terminal adequado.</translation>
    </message>
    <message>
        <source>CPU Governor</source>
        <translation>Governador de CPU</translation>
    </message>
    <message>
        <source>Select CPU Governor:</source>
        <translation>Selecione o Governador de CPU:</translation>
    </message>
    <message>
        <source>IPv6 Configuration</source>
        <translation>Configuração de IPv6</translation>
    </message>
    <message>
        <source>Enable or disable IPv6?</source>
        <translation>Habilitar ou desabilitar IPv6?</translation>
    </message>
    <message>
        <source>Apply TRIM</source>
        <translation>Aplicar TRIM</translation>
    </message>
    <message>
        <source>This will enable automatic TRIM for SSDs.

The fstrim.timer service will be enabled and started.

Continue?</source>
        <translation>Isto ativará o TRIM automático para SSDs.

O serviço fstrim.timer será habilitado e iniciado.

Continuar?</translation>
    </message>
    <message>
        <source>Apply tmpfs</source>
        <translation>Aplicar tmpfs</translation>
    </message>
    <message>
        <source>⚠️ WARNING: This will mount /tmp as tmpfs (RAM disk).

This requires editing /etc/fstab and a REBOOT.

All files in /tmp will be lost on reboot.

Continue?</source>
        <translation>⚠️ AVISO: Isto montará /tmp como tmpfs (disco RAM).

Isto requer editar o /etc/fstab e REINICIAR.

Todos os arquivos em /tmp serão perdidos ao reiniciar.

Continuar?</translation>
    </message>
    <message>
        <source>Manual Step Required</source>
        <translation>Passo Manual Necessário</translation>
    </message>
    <message>
        <source>The tmpfs configuration will be added to /etc/fstab.

You will need to REBOOT for this to take effect.

Opening /etc/fstab for editing...</source>
        <translation>A configuração de tmpfs será adicionada ao /etc/fstab.

Você precisará REINICIAR para que faça efeito.

Abrindo /etc/fstab para edição...</translation>
    </message>
    <message>
        <source>Next Steps</source>
        <translation>Próximos Passos</translation>
    </message>
    <message>
        <source>Add this line to /etc/fstab:

/tmp tmpfs tmpfs defaults,noatime,mode=1777 0 0

Then save and REBOOT.</source>
        <translation>Adicione esta linha ao /etc/fstab:

/tmp tmpfs tmpfs defaults,noatime,mode=1777 0 0

Depois salve e REINICIE.</translation>
    </message>
    <message>
        <source>DNS configuration requires manual editing.

Opening /etc/resolv.conf for editing...

Add nameserver lines like:
nameserver 8.8.8.8
nameserver 8.8.4.4</source>
        <translation>A configuração de DNS requer edição manual.

Abrindo /etc/resolv.conf para edição...

Adicione linhas nameserver como:
nameserver 8.8.8.8
nameserver 8.8.4.4</translation>
    </message>
    <message>
        <source>Applied</source>
        <translation>Aplicado</translation>
    </message>
    <message>
        <source>Hidden files will now be shown by default in Dolphin.

You may need to restart Dolphin for changes to take effect.</source>
        <translation>Arquivos ocultos agora serão mostrados por padrão no Dolphin.

Pode ser necessário reiniciar o Dolphin para que as mudanças façam efeito.</translation>
    </message>
    <message>
        <source>Security Warning</source>
        <translation>Aviso de Segurança</translation>
    </message>
    <message>
        <source>⚠️ WARNING: Disabling mitigations reduces security!

This will modify GRUB configuration.
You will need to update GRUB and REBOOT.

Do you want to DISABLE mitigations?</source>
        <translation>⚠️ AVISO: Desativar as mitigações reduz a segurança!

Isto modificará a configuração do GRUB.
Você precisará atualizar o GRUB e REINICIAR.

Deseja DESATIVAR as mitigações?</translation>
    </message>
    <message>
        <source>Opening /etc/default/grub for editing...

Add 'mitigations=off' to GRUB_CMDLINE_LINUX,
then run: sudo grub-mkconfig -o /boot/grub/grub.cfg
and REBOOT.</source>
        <translation>Abrindo /etc/default/grub para edição...

Adicione 'mitigations=off' em GRUB_CMDLINE_LINUX,
depois execute: sudo grub-mkconfig -o /boot/grub/grub.cfg
e REINICIE.</translation>
    </message>
    <message>
        <source>Performance Hacks</source>
        <translation>Hacks de Desempenho</translation>
    </message>
    <message>
        <source>These optimizations require multiple steps:

1. Install plocate: sudo pacman -S plocate
2. Edit /etc/updatedb.conf
3. Set performance mode: sudo powerprofilesctl set performance
4. Edit /etc/mkinitcpio.conf to remove plymouth
5. Run: sudo mkinitcpio -P

See Info button for detailed instructions.</source>
        <translation>Estas otimizações requerem vários passos:

1. Instale o plocate: sudo pacman -S plocate
2. Edite /etc/updatedb.conf
3. Ative o modo desempenho: sudo powerprofilesctl set performance
4. Edite /etc/mkinitcpio.conf para remover o plymouth
5. Execute: sudo mkinitcpio -P

Veja o botão Info para instruções detalhadas.</translation>
    </message>
    <message>
        <source>ptrace Configuration</source>
        <translation>Configuração do ptrace</translation>
    </message>
    <message>
        <source>Select ptrace scope:</source>
        <translation>Selecione o escopo do ptrace:</translation>
    </message>
    <message>
        <source>Security Configuration</source>
        <translation>Configuração de Segurança</translation>
    </message>
    <message>
        <source>⚠️ WARNING: This requires editing /etc/fstab and remounting /proc.

This will hide processes from other users.

Continue?</source>
        <translation>⚠️ AVISO: Isto requer editar o /etc/fstab e remontar o /proc.

Isto ocultará processos de outros usuários.

Continuar?</translation>
    </message>
    <message>
        <source>Opening /etc/fstab for editing...

Find the /proc line and add hidepid=2:
proc /proc proc defaults,hidepid=2 0 0

Then run: sudo mount -o remount /proc</source>
        <translation>Abrindo /etc/fstab para edição...

Encontre a linha do /proc e adicione hidepid=2:
proc /proc proc defaults,hidepid=2 0 0

Depois execute: sudo mount -o remount /proc</translation>
    </message>
    <message>
        <source>Swappiness Configuration</source>
        <translation>Configuração de Swappiness</translation>
    </message>
    <message>
        <source>Set swappiness value (1-100):</source>
        <translation>Defina o valor de swappiness (1-100):</translation>
    </message>
    <message>
        <source>Lower values = less swapping (recommended: 10-30)</source>
        <translation>Valores menores = menos swap (recomendado: 10-30)</translation>
    </message>
    <message>
        <source>I/O Scheduler Configuration</source>
        <translation>Configuração do Escalonador de E/S</translation>
    </message>
    <message>
        <source>Select I/O Scheduler:</source>
        <translation>Selecione o Escalonador de E/S:</translation>
    </message>
    <message>
        <source>none/mq-deadline/kyber for NVMe, mq-deadline/bfq for SATA</source>
        <translation>none/mq-deadline/kyber para NVMe, mq-deadline/bfq para SATA</translation>
    </message>
    <message>
        <source>Transparent Huge Pages</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Select THP mode:</source>
        <translation>Selecione o modo THP:</translation>
    </message>
    <message>
        <source>Apply TCP Optimizations</source>
        <translation>Aplicar Otimizações TCP</translation>
    </message>
    <message>
        <source>This will create TCP optimization settings.

These optimizations improve network performance.

Continue?</source>
        <translation>Isto criará configurações de otimização TCP.

Estas otimizações melhoram o desempenho da rede.

Continuar?</translation>
    </message>
    <message>
        <source>Firewall Configuration</source>
        <translation>Configuração de Firewall</translation>
    </message>
    <message>
        <source>Firewall configuration depends on which firewall you're using:

For firewalld:
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --reload

For ufw:
sudo ufw allow 22/tcp
sudo ufw reload</source>
        <translation>A configuração do firewall depende de qual você usa:

Para firewalld:
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --reload

Para ufw:
sudo ufw allow 22/tcp
sudo ufw reload</translation>
    </message>
    <message>
        <source>Select firewall to enable:</source>
        <translation>Selecione o firewall a habilitar:</translation>
    </message>
    <message>
        <source>Apply Pacman Optimizations</source>
        <translation>Aplicar Otimizações do Pacman</translation>
    </message>
    <message>
        <source>This will add optimizations to /etc/pacman.conf:

- ParallelDownloads = 10
- Color (if not already enabled)
- ILoveCandy (progress bar)

Continue?</source>
        <translation>Isto adicionará otimizações ao /etc/pacman.conf:

- ParallelDownloads = 10
- Color (se ainda não ativado)
- ILoveCandy (barra de progresso)

Continuar?</translation>
    </message>
    <message>
        <source>Manual Configuration</source>
        <translation>Configuração Manual</translation>
    </message>
    <message>
        <source>Opening /etc/pacman.conf for editing...

Add these lines in the [options] section:
Color
ParallelDownloads = 10
ILoveCandy</source>
        <translation>Abrindo /etc/pacman.conf para edição...

Adicione estas linhas na seção [options]:
Color
ParallelDownloads = 10
ILoveCandy</translation>
    </message>
    <message>
        <source>Apply Journald Settings</source>
        <translation>Aplicar Configurações do Journald</translation>
    </message>
    <message>
        <source>This will add log size limits to /etc/systemd/journald.conf:

SystemMaxUse=500M
SystemKeepFree=1G
SystemMaxFileSize=50M

Continue?</source>
        <translation>Isto adicionará limites de tamanho de log ao /etc/systemd/journald.conf:

SystemMaxUse=500M
SystemKeepFree=1G
SystemMaxFileSize=50M

Continuar?</translation>
    </message>
    <message>
        <source>Opening /etc/systemd/journald.conf for editing...

Uncomment and set:
SystemMaxUse=500M
SystemKeepFree=1G
SystemMaxFileSize=50M

Then restart: sudo systemctl restart systemd-journald</source>
        <translation>Abrindo /etc/systemd/journald.conf para edição...

Descomente e defina:
SystemMaxUse=500M
SystemKeepFree=1G
SystemMaxFileSize=50M

Depois reinicie: sudo systemctl restart systemd-journald</translation>
    </message>
    <message>
        <source>Apply Gaming Optimizations</source>
        <translation>Aplicar Otimizações para Jogos</translation>
    </message>
    <message>
        <source>This raises vm.max_map_count to 2147483642 (the SteamOS/CachyOS value).

Many Proton/Steam games need this to avoid crashes and stutter.
It is safe for normal desktop use.

Continue?</source>
        <translation>Isto aumenta vm.max_map_count para 2147483642 (o valor do SteamOS/CachyOS).

Muitos jogos Proton/Steam precisam disso para evitar crashes e travamentos.
É seguro para uso normal de desktop.

Continuar?</translation>
    </message>
    <message>
        <source>Turbo boost setting:</source>
        <translation>Configuração do turbo boost:</translation>
    </message>
    <message>
        <source>NMI Watchdog</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>NMI watchdog setting:</source>
        <translation>Configuração do NMI watchdog:</translation>
    </message>
    <message>
        <source>Core Dumps</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Core dump setting:</source>
        <translation>Configuração de core dump:</translation>
    </message>
    <message>
        <source>Apply Memory Cache Tuning</source>
        <translation>Aplicar Ajuste de Cache de Memória</translation>
    </message>
    <message>
        <source>This applies desktop-friendly memory settings:

vm.vfs_cache_pressure = 50
vm.dirty_ratio = 10
vm.dirty_background_ratio = 5

Faster file browsing and fewer stalls during big file copies.

Continue?</source>
        <translation>Isto aplica configurações de memória amigáveis para desktop:

vm.vfs_cache_pressure = 50
vm.dirty_ratio = 10
vm.dirty_background_ratio = 5

Navegação de arquivos mais rápida e menos travamentos em cópias grandes.

Continuar?</translation>
    </message>
    <message>
        <source>Apply Inotify Limits</source>
        <translation>Aplicar Limites do Inotify</translation>
    </message>
    <message>
        <source>This raises the file-watching limits:

fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 1024

Fixes 'unable to watch for changes' errors in IDEs,
sync tools, and some game launchers.

Continue?</source>
        <translation>Isto aumenta os limites de monitoramento de arquivos:

fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 1024

Corrige erros de 'unable to watch for changes' em IDEs,
ferramentas de sync e alguns launchers de jogos.

Continuar?</translation>
    </message>
    <message>
        <source>Alias</source>
        <translation>Alias</translation>
    </message>
    <message>
        <source>Command</source>
        <translation>Comando</translation>
    </message>
    <message>
        <source>Alias name:</source>
        <translation>Nome do alias:</translation>
    </message>
    <message>
        <source>Alias command:</source>
        <translation>Comando do alias:</translation>
    </message>
    <message>
        <source>Duplicate Alias</source>
        <translation>Alias Duplicado</translation>
    </message>
    <message>
        <source>An alias with this name already exists.</source>
        <translation>Já existe um alias com esse nome.</translation>
    </message>
    <message>
        <source>Are you sure you want to remove this alias?</source>
        <translation>Tem certeza de que deseja remover este alias?</translation>
    </message>
    <message>
        <source>Directory Creation Failed</source>
        <translation>Falha ao Criar Diretório</translation>
    </message>
    <message>
        <source>Failed to create output directory: </source>
        <translation>Falha ao criar o diretório de saída: </translation>
    </message>
    <message>
        <source>Sudo Password</source>
        <translation>Senha do Sudo</translation>
    </message>
    <message>
        <source>Enter your sudo password:</source>
        <translation>Digite sua senha do sudo:</translation>
    </message>
    <message>
        <source>Success</source>
        <translation>Sucesso</translation>
    </message>
    <message>
        <source>System clone ISO has been created successfully!

The ISO contains an exact copy of your current system and can be used to install it on other machines.</source>
        <translation>A ISO de clone do sistema foi criada com sucesso!

A ISO contém uma cópia exata do seu sistema atual e pode ser usada para instalá-lo em outras máquinas.</translation>
    </message>
    <message>
        <source>Failed to create ISO. Check the output above for details.

Exit code: </source>
        <translation>Falha ao criar a ISO. Verifique a saída acima para detalhes.

Código de saída: </translation>
    </message>
    <message>
        <source>Process Error</source>
        <translation>Erro de Processo</translation>
    </message>
    <message>
        <source>Error running ISO creation script: </source>
        <translation>Erro ao executar o script de criação da ISO: </translation>
    </message>
    <message>
        <source>Failed to create temporary script file</source>
        <translation>Falha ao criar o arquivo de script temporário</translation>
    </message>
    <message>
        <source>❌ Offline package not found. Click download button to get it.</source>
        <translation>❌ Pacote offline não encontrado. Clique no botão de download para obtê-lo.</translation>
    </message>
    <message>
        <source>⚠️ ISO creation needs these missing tools:</source>
        <translation>⚠️ A criação de ISO precisa destas ferramentas ausentes:</translation>
    </message>
    <message>
        <source>⬇️ Install %1</source>
        <translation>⬇️ Instalar %1</translation>
    </message>
    <message>
        <source>⬇️ Install All</source>
        <translation>⬇️ Instalar Tudo</translation>
    </message>
    <message>
        <source>Loading installed software...</source>
        <translation>Carregando software instalado...</translation>
    </message>
    <message>
        <source>❌ Failed to read installed packages.</source>
        <translation>❌ Falha ao ler os pacotes instalados.</translation>
    </message>
    <message>
        <source>Multiple kernels = safety. Keep an LTS fallback next to your daily driver.</source>
        <translation>Vários kernels = segurança. Mantenha um LTS de reserva além do seu kernel do dia a dia.</translation>
    </message>
    <message>
        <source>⬇️ Install</source>
        <translation>⬇️ Instalar</translation>
    </message>
    <message>
        <source>🗑️ Remove Selected</source>
        <translation>🗑️ Remover Selecionado</translation>
    </message>
    <message>
        <source>Version</source>
        <translation>Versão</translation>
    </message>
    <message>
        <source>Kernel Package</source>
        <translation type="vanished">Pacote do Kernel</translation>
    </message>
    <message>
        <source>Running</source>
        <translation>Em Execução</translation>
    </message>
    <message>
        <source>Headers</source>
        <translation>Headers</translation>
    </message>
    <message>
        <source>Boot Image</source>
        <translation>Imagem de Boot</translation>
    </message>
    <message>
        <source>🥾 Boot entry (GRUB)</source>
        <translation>🥾 Entrada de boot (GRUB)</translation>
    </message>
    <message>
        <source>⭐ Set Selected as Default Boot Entry</source>
        <translation>⭐ Definir Selecionada como Boot Padrão</translation>
    </message>
    <message>
        <source>🔧 Regenerate GRUB Config</source>
        <translation>🔧 Regenerar Config do GRUB</translation>
    </message>
    <message>
        <source>Nothing to Install</source>
        <translation type="vanished">Nada para Instalar</translation>
    </message>
    <message>
        <source>All known kernels are already installed.</source>
        <translation type="vanished">Todos os kernels conhecidos já estão instalados.</translation>
    </message>
    <message>
        <source>Select an installed kernel first.</source>
        <translation>Selecione um kernel instalado primeiro.</translation>
    </message>
    <message>
        <source>'%1' is the kernel you are running right now. Boot another kernel first.</source>
        <translation>'%1' é o kernel em execução agora. Inicie outro kernel primeiro.</translation>
    </message>
    <message>
        <source>That is your only installed kernel.</source>
        <translation>Esse é seu único kernel instalado.</translation>
    </message>
    <message>
        <source>Remove Kernel</source>
        <translation>Remover Kernel</translation>
    </message>
    <message>
        <source>Remove '%1' and its headers?</source>
        <translation type="vanished">Remover '%1' e seus headers?</translation>
    </message>
    <message>
        <source>Select a boot entry from the list first.</source>
        <translation>Selecione uma entrada de boot na lista primeiro.</translation>
    </message>
    <message>
        <source>Set Default Boot Entry</source>
        <translation>Definir Boot Padrão</translation>
    </message>
    <message>
        <source>Boot this entry by default from now on?

%1</source>
        <translation>Iniciar esta entrada por padrão de agora em diante?

%1</translation>
    </message>
    <message>
        <source>✅ current (%1)</source>
        <translation>✅ atual (%1)</translation>
    </message>
    <message>
        <source>✅ installed</source>
        <translation>✅ instalado</translation>
    </message>
    <message>
        <source>🌟 Install Popular Kernel...</source>
        <translation>🌟 Instalar Kernel Popular...</translation>
    </message>
    <message>
        <source>Official repo</source>
        <translation>Repositório oficial</translation>
    </message>
    <message>
        <source>Vanilla Arch kernel</source>
        <translation>Kernel Arch original</translation>
    </message>
    <message>
        <source>Long-term support — the safe fallback</source>
        <translation>Suporte de longo prazo — a reserva segura</translation>
    </message>
    <message>
        <source>Desktop/gaming tuned (zen patches)</source>
        <translation>Otimizado para desktop/jogos (patches zen)</translation>
    </message>
    <message>
        <source>Security-hardened</source>
        <translation>Reforçado em segurança</translation>
    </message>
    <message>
        <source>Realtime preemption (audio/robotics)</source>
        <translation>Preempção em tempo real (áudio/robótica)</translation>
    </message>
    <message>
        <source>CachyOS repo / AUR</source>
        <translation>Repo CachyOS / AUR</translation>
    </message>
    <message>
        <source>BORE scheduler + CachyOS optimizations</source>
        <translation>Escalonador BORE + otimizações CachyOS</translation>
    </message>
    <message>
        <source>AUR</source>
        <translation>AUR</translation>
    </message>
    <message>
        <source>XanMod performance build</source>
        <translation>Build de desempenho XanMod</translation>
    </message>
    <message>
        <source>Latest mainline release candidate</source>
        <translation>Última release candidate da mainline</translation>
    </message>
    <message>
        <source>GitHub — build from source</source>
        <translation>GitHub — compilar do código-fonte</translation>
    </message>
    <message>
        <source>Frogging-Family TKG: interactive, fully customizable build</source>
        <translation>Frogging-Family TKG: build interativa e totalmente personalizável</translation>
    </message>
    <message>
        <source>Install Popular Kernel</source>
        <translation>Instalar Kernel Popular</translation>
    </message>
    <message>
        <source>Pick a kernel — repo installs use pacman, AUR uses yay/paru, and TKG clones the
GitHub repo and runs its interactive makepkg build (you choose the options in the terminal).</source>
        <translation>Escolha um kernel — instalações do repo usam pacman, AUR usa yay/paru, e o TKG clona o
repositório do GitHub e roda a build interativa do makepkg (você escolhe as opções no terminal).</translation>
    </message>
    <message>
        <source>Not Package-Managed</source>
        <translation>Não Gerenciado por Pacote</translation>
    </message>
    <message>
        <source>'%1' was not installed by pacman, so it must be removed by hand:

  sudo rm /boot/vmlinuz-%1
  sudo rm /boot/initramfs-%1*.img
  sudo rm -r /usr/lib/modules/&lt;its version&gt;

Then Regenerate GRUB Config. Double-check each path before deleting!</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Remove package '%1' (boot image '%2') and its headers?

pacman + the mkinitcpio hook clean up /boot and the modules; GRUB is regenerated automatically afterwards.</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>—</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>(unknown)</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>GRUB_DEFAULT is currently: %1 — select an entry below and set it as default.</source>
        <translation>GRUB_DEFAULT atual: %1 — selecione uma entrada abaixo e defina como padrão.</translation>
    </message>
    <message>
        <source>Source</source>
        <translation>Origem</translation>
    </message>
    <message>
        <source>Installed</source>
        <translation>Instalado</translation>
    </message>
    <message>
        <source>No Selection</source>
        <translation>Nada Selecionado</translation>
    </message>
    <message>
        <source>Please select one or more packages to uninstall.</source>
        <translation>Selecione um ou mais pacotes para desinstalar.</translation>
    </message>
    <message>
        <source>Uninstall running in terminal — the list will refresh automatically when it finishes.</source>
        <translation>Desinstalação em execução no terminal — a lista será atualizada automaticamente ao terminar.</translation>
    </message>
    <message>
        <source>Authentication Failed</source>
        <translation>Falha na Autenticação</translation>
    </message>
    <message>
        <source>The sudo password was not accepted.</source>
        <translation>A senha do sudo não foi aceita.</translation>
    </message>
    <message>
        <source>Please select a disk to health-check.</source>
        <translation>Selecione um disco para verificar a saúde.</translation>
    </message>
    <message>
        <source>Health check works on disks, not partitions. Select the disk itself.</source>
        <translation>A verificação de saúde funciona em discos, não partições. Selecione o disco em si.</translation>
    </message>
    <message>
        <source>Health Check Failed</source>
        <translation>Verificação de Saúde Falhou</translation>
    </message>
    <message>
        <source>Could not read SMART data (is smartmontools installed?).</source>
        <translation>Não foi possível ler os dados SMART (smartmontools está instalado?).</translation>
    </message>
    <message>
        <source>Select the USB disk to burn to (the disk, not a partition).</source>
        <translation>Selecione o disco USB para gravar (o disco, não uma partição).</translation>
    </message>
    <message>
        <source>Select the whole disk (type 'disk'), not a partition.</source>
        <translation>Selecione o disco inteiro (tipo 'disk'), não uma partição.</translation>
    </message>
    <message>
        <source>Refused</source>
        <translation>Recusado</translation>
    </message>
    <message>
        <source>That is the disk your running system is installed on. Burning to it would destroy this system.</source>
        <translation>Esse é o disco onde o sistema em execução está instalado. Gravar nele destruiria este sistema.</translation>
    </message>
    <message>
        <source>Cancelled</source>
        <translation>Cancelado</translation>
    </message>
    <message>
        <source>Device path did not match — nothing was written.</source>
        <translation>O caminho do dispositivo não correspondeu — nada foi gravado.</translation>
    </message>
    <message>
        <source>✅ No failed services</source>
        <translation>✅ Nenhum serviço com falha</translation>
    </message>
    <message>
        <source>Boot Time Analysis</source>
        <translation>Análise de Tempo de Boot</translation>
    </message>
    <message>
        <source>Select a config file in the table first.</source>
        <translation>Selecione primeiro um arquivo de config na tabela.</translation>
    </message>
    <message>
        <source>Directory</source>
        <translation>Diretório</translation>
    </message>
    <message>
        <source>That entry is a directory — pick a specific file inside it (use the terminal editor for browsing).</source>
        <translation>Essa entrada é um diretório — escolha um arquivo específico dentro dele (use o editor no terminal para navegar).</translation>
    </message>
    <message>
        <source>Read Failed</source>
        <translation>Falha na Leitura</translation>
    </message>
    <message>
        <source>Could not read the file even with sudo.</source>
        <translation>Não foi possível ler o arquivo nem com sudo.</translation>
    </message>
    <message>
        <source>A timestamped backup is saved automatically before every save.</source>
        <translation>Um backup com data/hora é salvo automaticamente antes de cada salvamento.</translation>
    </message>
    <message>
        <source>❌ Could not write file.</source>
        <translation>❌ Não foi possível gravar o arquivo.</translation>
    </message>
    <message>
        <source>⏸ Stop Following</source>
        <translation>⏸ Parar de Seguir</translation>
    </message>
    <message>
        <source>Following journal (live)...</source>
        <translation>Seguindo o journal (ao vivo)...</translation>
    </message>
    <message>
        <source>Stopped following.</source>
        <translation>Parou de seguir.</translation>
    </message>
    <message>
        <source>Loading journal...</source>
        <translation>Carregando journal...</translation>
    </message>
    <message>
        <source>Package</source>
        <translation>Pacote</translation>
    </message>
    <message>
        <source>New Version</source>
        <translation>Nova Versão</translation>
    </message>
    <message>
        <source>Current Version</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Checking official repos...</source>
        <translation>Verificando repositórios oficiais...</translation>
    </message>
    <message>
        <source>⚠️ 'checkupdates' not found — install it with: sudo pacman -S pacman-contrib</source>
        <translation>⚠️ 'checkupdates' não encontrado — instale com: sudo pacman -S pacman-contrib</translation>
    </message>
    <message>
        <source>⚠️ checkupdates failed (install 'pacman-contrib').</source>
        <translation>⚠️ checkupdates falhou (instale 'pacman-contrib').</translation>
    </message>
    <message>
        <source>✅ System is up to date!</source>
        <translation>✅ O sistema está atualizado!</translation>
    </message>
    <message>
        <source>Upgrade System</source>
        <translation>Atualizar Sistema</translation>
    </message>
    <message>
        <source>Run a full system upgrade (sudo pacman -Syu) in a terminal?

Tip: check the Arch news list first — occasionally upgrades need manual steps.</source>
        <translation>Executar uma atualização completa do sistema (sudo pacman -Syu) em um terminal?

Dica: confira antes a lista de notícias do Arch — às vezes atualizações exigem passos manuais.</translation>
    </message>
    <message>
        <source>No AUR Helper</source>
        <translation>Sem Helper do AUR</translation>
    </message>
    <message>
        <source>Install Yay or Paru first (PKG Install tab).</source>
        <translation>Instale o Yay ou Paru primeiro (aba Instalar PKG).</translation>
    </message>
    <message>
        <source>No Orphans</source>
        <translation>Sem Órfãos</translation>
    </message>
    <message>
        <source>🎉 No orphaned packages found — nothing to clean.</source>
        <translation>🎉 Nenhum pacote órfão encontrado — nada para limpar.</translation>
    </message>
    <message>
        <source>Orphan cleanup running in terminal — click Refresh when done.</source>
        <translation>Limpeza de órfãos em execução no terminal — clique em Atualizar ao terminar.</translation>
    </message>
    <message>
        <source>Protocol</source>
        <translation>Protocolo</translation>
    </message>
    <message>
        <source>Address</source>
        <translation>Endereço</translation>
    </message>
    <message>
        <source>Port</source>
        <translation>Porta</translation>
    </message>
    <message>
        <source>Process</source>
        <translation>Processo</translation>
    </message>
    <message>
        <source>PID</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>⏳ Scanning home folder sizes (first scan can take a minute)...</source>
        <translation>⏳ Escaneando tamanhos das pastas do home (o primeiro escaneamento pode levar um minuto)...</translation>
    </message>
    <message>
        <source>Nothing to Save</source>
        <translation>Nada para Salvar</translation>
    </message>
    <message>
        <source>No exclusions are checked yet.
Check some folders/files first, then save.</source>
        <translation>Nenhuma exclusão está marcada ainda.
Marque algumas pastas/arquivos primeiro e depois salve.</translation>
    </message>
    <message>
        <source>Variable:</source>
        <translation>Variável:</translation>
    </message>
    <message>
        <source>Value:</source>
        <translation>Valor:</translation>
    </message>
    <message>
        <source>Like the Windows Environment Variables dialog. Changes apply at the NEXT login.</source>
        <translation>Como o diálogo de Variáveis de Ambiente do Windows. Mudanças valem no PRÓXIMO login.</translation>
    </message>
    <message>
        <source>Variable</source>
        <translation>Variável</translation>
    </message>
    <message>
        <source>Value</source>
        <translation>Valor</translation>
    </message>
    <message>
        <source>➕ Add</source>
        <translation>➕ Adicionar</translation>
    </message>
    <message>
        <source>✏️ Edit</source>
        <translation>✏️ Editar</translation>
    </message>
    <message>
        <source>🗑️ Remove</source>
        <translation>🗑️ Remover</translation>
    </message>
    <message>
        <source>Add Variable</source>
        <translation>Adicionar Variável</translation>
    </message>
    <message>
        <source>Select a variable first.</source>
        <translation>Selecione uma variável primeiro.</translation>
    </message>
    <message>
        <source>Edit Variable</source>
        <translation>Editar Variável</translation>
    </message>
    <message>
        <source>🖥️ System variables — /etc/environment (all users)</source>
        <translation>🖥️ Variáveis do sistema — /etc/environment (todos os usuários)</translation>
    </message>
    <message>
        <source>👤 User variables — ~/.config/environment.d (this user)</source>
        <translation>👤 Variáveis do usuário — ~/.config/environment.d (este usuário)</translation>
    </message>
    <message>
        <source>Save Failed</source>
        <translation>Falha ao Salvar</translation>
    </message>
    <message>
        <source>Could not write %1</source>
        <translation>Não foi possível gravar %1</translation>
    </message>
    <message>
        <source>Saved</source>
        <translation>Salvo</translation>
    </message>
    <message>
        <source>User variables saved. They apply at your next login.</source>
        <translation>Variáveis do usuário salvas. Valem no seu próximo login.</translation>
    </message>
    <message>
        <source>Could not write to:
</source>
        <translation>Não foi possível gravar em:
</translation>
    </message>
    <message>
        <source>Load Failed</source>
        <translation>Falha ao Carregar</translation>
    </message>
    <message>
        <source>Could not read:
</source>
        <translation>Não foi possível ler:
</translation>
    </message>
    <message>
        <source>Invalid Profile</source>
        <translation>Perfil Inválido</translation>
    </message>
    <message>
        <source>This file is not a valid exclusion profile.</source>
        <translation>Este arquivo não é um perfil de exclusões válido.</translation>
    </message>
    <message>
        <source>Select which tabs to show:</source>
        <translation>Selecione quais abas mostrar:</translation>
    </message>
    <message>
        <source>Drive Tools preferences will be added here</source>
        <translation>As preferências de Ferramentas de Disco serão adicionadas aqui</translation>
    </message>
    <message>
        <source>Shell Tools preferences will be added here</source>
        <translation>As preferências de Ferramentas de Shell serão adicionadas aqui</translation>
    </message>
    <message>
        <source>Package Manager preferences will be added here</source>
        <translation>As preferências do Gerenciador de Pacotes serão adicionadas aqui</translation>
    </message>
    <message>
        <source>Network preferences will be added here</source>
        <translation>As preferências de Rede serão adicionadas aqui</translation>
    </message>
    <message>
        <source>OK</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>About Arch OS Tools</source>
        <translation>Sobre o Arch OS Tools</translation>
    </message>
    <message>
        <source>Not available</source>
        <translation>Não disponível</translation>
    </message>
    <message>
        <source>Not configured</source>
        <translation>Não configurado</translation>
    </message>
    <message>
        <source>Cannot read</source>
        <translation>Não foi possível ler</translation>
    </message>
    <message>
        <source>Unable to fetch</source>
        <translation>Não foi possível obter</translation>
    </message>
    <message>
        <source>Create Bridge</source>
        <translation>Criar Bridge</translation>
    </message>
    <message>
        <source>NetworkManager (nmcli) is required to create a managed bridge.
Install it (e.g. pacman -S networkmanager).</source>
        <translation>O NetworkManager (nmcli) é necessário para criar uma bridge gerenciada.
Instale-o (ex.: pacman -S networkmanager).</translation>
    </message>
    <message>
        <source>No Ethernet interface found. Connect a cable or check your hardware.</source>
        <translation>Nenhuma interface Ethernet encontrada. Conecte um cabo ou verifique o hardware.</translation>
    </message>
    <message>
        <source>Create Bridge (NetworkManager)</source>
        <translation>Criar Bridge (NetworkManager)</translation>
    </message>
    <message>
        <source>Creates a bridge managed by NetworkManager with DHCP. The selected Ethernet interface will be attached so the bridge gets an IP. Use this for VM networking (e.g. libvirt).</source>
        <translation>Cria uma bridge gerenciada pelo NetworkManager com DHCP. A interface Ethernet selecionada será anexada para que a bridge receba um IP. Use para rede de VMs (ex.: libvirt).</translation>
    </message>
    <message>
        <source>Bridge name:</source>
        <translation>Nome da bridge:</translation>
    </message>
    <message>
        <source>br0</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Interface to attach:</source>
        <translation>Interface a anexar:</translation>
    </message>
    <message>
        <source>Create</source>
        <translation>Criar</translation>
    </message>
    <message>
        <source>Please select a bridge to delete.</source>
        <translation>Selecione uma bridge para excluir.</translation>
    </message>
    <message>
        <source>Could not create temporary script.</source>
        <translation>Não foi possível criar o script temporário.</translation>
    </message>
    <message>
        <source>Please select a network to start.</source>
        <translation>Selecione uma rede para iniciar.</translation>
    </message>
    <message>
        <source>Please select a network to stop.</source>
        <translation>Selecione uma rede para parar.</translation>
    </message>
    <message>
        <source>Please select a network to restart.</source>
        <translation>Selecione uma rede para reiniciar.</translation>
    </message>
    <message>
        <source>Please select an interface to configure.</source>
        <translation>Selecione uma interface para configurar.</translation>
    </message>
    <message>
        <source>IP Address:</source>
        <translation>Endereço IP:</translation>
    </message>
    <message>
        <source>192.168.1.100</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Netmask:</source>
        <translation>Máscara de rede:</translation>
    </message>
    <message>
        <source>255.255.255.0</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Gateway:</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>192.168.1.1</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>8.8.8.8, 8.8.4.4</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Please select an interface to bring up.</source>
        <translation>Selecione uma interface para ativar.</translation>
    </message>
    <message>
        <source>Please select an interface to bring down.</source>
        <translation>Selecione uma interface para desativar.</translation>
    </message>
    <message>
        <source>In Use</source>
        <translation>Em Uso</translation>
    </message>
    <message>
        <source>Signal</source>
        <translation>Sinal</translation>
    </message>
    <message>
        <source>Security</source>
        <translation>Segurança</translation>
    </message>
    <message>
        <source>Channel</source>
        <translation>Canal</translation>
    </message>
    <message>
        <source>Rate</source>
        <translation>Taxa</translation>
    </message>
    <message>
        <source>SSID</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>No Wi-Fi adapter detected.</source>
        <translation>Nenhum adaptador Wi-Fi detectado.</translation>
    </message>
    <message>
        <source>Scanning...</source>
        <translation>Escaneando...</translation>
    </message>
    <message>
        <source>❌ Failed to list Wi-Fi networks.</source>
        <translation>❌ Falha ao listar redes Wi-Fi.</translation>
    </message>
    <message>
        <source>Please select a Wi-Fi network to connect to.</source>
        <translation>Selecione uma rede Wi-Fi para conectar.</translation>
    </message>
    <message>
        <source>No Wi-Fi</source>
        <translation>Sem Wi-Fi</translation>
    </message>
    <message>
        <source>Name</source>
        <translation>Nome</translation>
    </message>
    <message>
        <source>Device</source>
        <translation>Dispositivo</translation>
    </message>
    <message>
        <source>Autoconnect</source>
        <translation>Autoconectar</translation>
    </message>
    <message>
        <source>Active</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Please select a connection to activate.</source>
        <translation>Selecione uma conexão para ativar.</translation>
    </message>
    <message>
        <source>Please select a connection to deactivate.</source>
        <translation>Selecione uma conexão para desativar.</translation>
    </message>
    <message>
        <source>Please select a connection to delete.</source>
        <translation>Selecione uma conexão para excluir.</translation>
    </message>
    <message>
        <source>Please select a connection.</source>
        <translation>Selecione uma conexão.</translation>
    </message>
    <message>
        <source>Restart NetworkManager</source>
        <translation>Reiniciar NetworkManager</translation>
    </message>
    <message>
        <source>Restart the NetworkManager service?

All connections will briefly drop and reconnect.</source>
        <translation>Reiniciar o serviço NetworkManager?

Todas as conexões cairão brevemente e reconectarão.</translation>
    </message>
    <message>
        <source>Yay: ✅ Installed</source>
        <translation>Yay: ✅ Instalado</translation>
    </message>
    <message>
        <source>Yay: ❌ Not Installed</source>
        <translation>Yay: ❌ Não Instalado</translation>
    </message>
    <message>
        <source>Paru: ✅ Installed</source>
        <translation>Paru: ✅ Instalado</translation>
    </message>
    <message>
        <source>Paru: ❌ Not Installed</source>
        <translation>Paru: ❌ Não Instalado</translation>
    </message>
    <message>
        <source>Repository</source>
        <translation>Repositório</translation>
    </message>
    <message>
        <source>Votes</source>
        <translation>Votos</translation>
    </message>
    <message>
        <source>Clear Cache</source>
        <translation>Limpar Cache</translation>
    </message>
    <message>
        <source>This will clear the pacman package cache.

This action requires sudo privileges.

Do you want to continue?</source>
        <translation>Isto limpará o cache de pacotes do pacman.

Esta ação requer privilégios sudo.

Deseja continuar?</translation>
    </message>
    <message>
        <source>Please select a package to uninstall.</source>
        <translation>Selecione um pacote para desinstalar.</translation>
    </message>
    <message>
        <source>Please select a package to reinstall.</source>
        <translation>Selecione um pacote para reinstalar.</translation>
    </message>
    <message>
        <source>Please select a package to install.</source>
        <translation>Selecione um pacote para instalar.</translation>
    </message>
    <message>
        <source>AUR package requires Yay or Paru to be installed. Please install one to install AUR packages.</source>
        <translation>Pacotes do AUR exigem Yay ou Paru instalados. Instale um deles para instalar pacotes do AUR.</translation>
    </message>
    <message>
        <source>This will install Yay AUR helper.

This requires sudo privileges and will clone from GitHub.

Do you want to continue?</source>
        <translation>Isto instalará o helper AUR Yay.

Requer privilégios sudo e clonará do GitHub.

Deseja continuar?</translation>
    </message>
    <message>
        <source>This will uninstall Yay AUR helper.

This requires sudo privileges.

Do you want to continue?</source>
        <translation>Isto desinstalará o helper AUR Yay.

Requer privilégios sudo.

Deseja continuar?</translation>
    </message>
    <message>
        <source>This will install Paru AUR helper.

This requires sudo privileges and will clone from GitHub.

Do you want to continue?</source>
        <translation>Isto instalará o helper AUR Paru.

Requer privilégios sudo e clonará do GitHub.

Deseja continuar?</translation>
    </message>
    <message>
        <source>This will uninstall Paru AUR helper.

This requires sudo privileges.

Do you want to continue?</source>
        <translation>Isto desinstalará o helper AUR Paru.

Requer privilégios sudo.

Deseja continuar?</translation>
    </message>
    <message>
        <source>Choose a Debian package</source>
        <translation>Escolher um pacote Debian</translation>
    </message>
    <message>
        <source>Debian packages (*.deb)</source>
        <translation>Pacotes Debian (*.deb)</translation>
    </message>
    <message>
        <source>debtap Not Installed</source>
        <translation>debtap Não Instalado</translation>
    </message>
    <message>
        <source>Converting .deb files needs 'debtap' (AUR), and no AUR helper was found.

Install Yay or Paru first (buttons above), then try again.</source>
        <translation>Converter arquivos .deb exige o 'debtap' (AUR), e nenhum helper do AUR foi encontrado.

Instale o Yay ou Paru primeiro (botões acima) e tente novamente.</translation>
    </message>
    <message>
        <source>Install debtap?</source>
        <translation>Instalar debtap?</translation>
    </message>
    <message>
        <source>Converting .deb files needs 'debtap' (AUR package).

Install it now with %1?</source>
        <translation>Converter arquivos .deb exige o 'debtap' (pacote do AUR).

Instalar agora com %1?</translation>
    </message>
    <message>
        <source>Heads-up</source>
        <translation>Atenção</translation>
    </message>
    <message>
        <source>Two things worth knowing:

• Check the AUR first — most software shipped as .deb has a proper Arch package already.
• Self-contained apps convert well; packages deeply tied to Debian's libraries may not. If the conversion fails, a Debian Distrobox is the robust alternative.

The terminal will show the entire conversion — debtap may ask you to confirm the package name and license (pressing Enter accepts its suggestion).</source>
        <translation>Duas coisas que vale saber:

• Verifique o AUR primeiro — a maioria dos softwares distribuídos como .deb já tem um pacote Arch adequado.
• Apps autocontidos convertem bem; pacotes muito ligados às bibliotecas do Debian podem não converter. Se a conversão falhar, um Distrobox do Debian é a alternativa robusta.

O terminal mostrará toda a conversão — o debtap pode pedir para confirmar o nome do pacote e a licença (Enter aceita a sugestão).</translation>
    </message>
    <message>
        <source>Service</source>
        <translation>Serviço</translation>
    </message>
    <message>
        <source>Enabled</source>
        <translation>Habilitado</translation>
    </message>
    <message>
        <source>SubState</source>
        <translation>Subestado</translation>
    </message>
    <message>
        <source>Snapshots protect you from bad updates — create one before big changes.</source>
        <translation>Snapshots te protegem de atualizações ruins — crie um antes de grandes mudanças.</translation>
    </message>
    <message>
        <source>🔓 Load Snapshots (sudo)</source>
        <translation>🔓 Carregar Snapshots (sudo)</translation>
    </message>
    <message>
        <source>📸 Create Restore Point</source>
        <translation>📸 Criar Ponto de Restauração</translation>
    </message>
    <message>
        <source>⏪ How to Roll Back</source>
        <translation>⏪ Como Reverter</translation>
    </message>
    <message>
        <source>#</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Date</source>
        <translation>Data</translation>
    </message>
    <message>
        <source>Create Restore Point</source>
        <translation>Criar Ponto de Restauração</translation>
    </message>
    <message>
        <source>Description:</source>
        <translation>Descrição:</translation>
    </message>
    <message>
        <source>Manual restore point</source>
        <translation>Ponto de restauração manual</translation>
    </message>
    <message>
        <source>Load and select a snapshot first.</source>
        <translation>Carregue e selecione um snapshot primeiro.</translation>
    </message>
    <message>
        <source>Snapshot 0 is the live system itself.</source>
        <translation>O snapshot 0 é o próprio sistema em execução.</translation>
    </message>
    <message>
        <source>Delete Snapshot</source>
        <translation>Excluir Snapshot</translation>
    </message>
    <message>
        <source>Delete snapshot #%1? The disk space is reclaimed.</source>
        <translation>Excluir o snapshot #%1? O espaço em disco é recuperado.</translation>
    </message>
    <message>
        <source>Rolling Back</source>
        <translation>Revertendo</translation>
    </message>
    <message>
        <source>Rolling back is deliberately not a one-click action — it replaces your running system.

The safe ways:

1. If the system still boots:
     sudo snapper -c root rollback &lt;number&gt;
     then reboot.

2. If the system does not boot and grub-btrfs is installed:
     pick the snapshot from the GRUB menu ('Arch snapshots'),
     boot into it, then run the rollback command above.

3. Nuclear option: boot your Safety ISO from the System ISO tab.

Files-only recovery: snapshots are browsable at /.snapshots/&lt;number&gt;/snapshot/ —
copy individual files back without rolling back anything.</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>snapper is not installed — snapshots need a btrfs filesystem + snapper.</source>
        <translation>snapper não está instalado — snapshots precisam de btrfs + snapper.</translation>
    </message>
    <message>
        <source>Your root filesystem IS btrfs — you just need snapper:

  sudo pacman -S snapper snap-pac grub-btrfs
  sudo snapper -c root create-config /

snap-pac then creates automatic before/after snapshots on every pacman operation.</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Your root filesystem is '%1', not btrfs, so snapper snapshots are unavailable.

Your alternative safety nets: the System ISO tab (full clone) and the Backup tab.</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Snapshots Not Set Up</source>
        <translation>Snapshots Não Configurados</translation>
    </message>
    <message>
        <source>snapper has no 'root' config yet — run: sudo snapper -c root create-config /</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>%1 snapshot(s). Automatic ones come from snap-pac; # 0 is the live system.</source>
        <translation>%1 snapshot(s). Os automáticos vêm do snap-pac; nº 0 é o sistema em execução.</translation>
    </message>
    <message>
        <source>Click Load to detect your firewall and list its rules.</source>
        <translation>Clique em Carregar para detectar seu firewall e listar as regras.</translation>
    </message>
    <message>
        <source>🔓 Load Rules (sudo)</source>
        <translation>🔓 Carregar Regras (sudo)</translation>
    </message>
    <message>
        <source>🟢 Enable Firewall</source>
        <translation>🟢 Ativar Firewall</translation>
    </message>
    <message>
        <source>🔴 Disable Firewall</source>
        <translation>🔴 Desativar Firewall</translation>
    </message>
    <message>
        <source>➕ Add Rule</source>
        <translation>➕ Adicionar Regra</translation>
    </message>
    <message>
        <source>🗑️ Delete Rule</source>
        <translation>🗑️ Excluir Regra</translation>
    </message>
    <message>
        <source>Rule</source>
        <translation>Regra</translation>
    </message>
    <message>
        <source>Disable Firewall</source>
        <translation>Desativar Firewall</translation>
    </message>
    <message>
        <source>Disable the firewall? All ports become reachable from the network.</source>
        <translation>Desativar o firewall? Todas as portas ficarão acessíveis pela rede.</translation>
    </message>
    <message>
        <source>Add Firewall Rule</source>
        <translation>Adicionar Regra de Firewall</translation>
    </message>
    <message>
        <source>e.g. 22, 8080, 6000:6100</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>both</source>
        <translation>ambos</translation>
    </message>
    <message>
        <source>anywhere (empty) or IP/subnet, e.g. 192.168.1.0/24</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Action:</source>
        <translation>Ação:</translation>
    </message>
    <message>
        <source>Port:</source>
        <translation>Porta:</translation>
    </message>
    <message>
        <source>Protocol:</source>
        <translation>Protocolo:</translation>
    </message>
    <message>
        <source>From:</source>
        <translation>De:</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Adicionar</translation>
    </message>
    <message>
        <source>Invalid Port</source>
        <translation>Porta Inválida</translation>
    </message>
    <message>
        <source>Enter a port number or range like 6000:6100.</source>
        <translation>Digite um número de porta ou intervalo como 6000:6100.</translation>
    </message>
    <message>
        <source>Invalid Source</source>
        <translation>Origem Inválida</translation>
    </message>
    <message>
        <source>Enter an IP or subnet like 192.168.1.0/24.</source>
        <translation>Digite um IP ou sub-rede como 192.168.1.0/24.</translation>
    </message>
    <message>
        <source>Load and select a rule first.</source>
        <translation>Carregue e selecione uma regra primeiro.</translation>
    </message>
    <message>
        <source>Delete Rule</source>
        <translation>Excluir Regra</translation>
    </message>
    <message>
        <source>Delete rule #%1?

%2</source>
        <translation>Excluir a regra #%1?

%2</translation>
    </message>
    <message>
        <source>No firewall installed.</source>
        <translation>Nenhum firewall instalado.</translation>
    </message>
    <message>
        <source>No Firewall</source>
        <translation>Sem Firewall</translation>
    </message>
    <message>
        <source>Neither ufw nor firewalld is installed.

Recommended for desktops:
  sudo pacman -S ufw
  sudo ufw enable

Then come back here to manage rules.</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>ufw — %1</source>
        <translation>ufw — %1</translation>
    </message>
    <message>
        <source>🟢 active</source>
        <translation>🟢 ativo</translation>
    </message>
    <message>
        <source>🔴 inactive</source>
        <translation>🔴 inativo</translation>
    </message>
    <message>
        <source>firewalld — default zone rules (ports shown are deletable here)</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>✏️ Change Hostname</source>
        <translation>✏️ Alterar Hostname</translation>
    </message>
    <message>
        <source>🆔 Regenerate machine-id</source>
        <translation>🆔 Regenerar machine-id</translation>
    </message>
    <message>
        <source>📋 Copy Info</source>
        <translation>📋 Copiar Info</translation>
    </message>
    <message>
        <source>Change Hostname</source>
        <translation>Alterar Hostname</translation>
    </message>
    <message>
        <source>New hostname (letters, digits, hyphens):</source>
        <translation>Novo hostname (letras, dígitos, hífens):</translation>
    </message>
    <message>
        <source>Invalid Hostname</source>
        <translation>Hostname Inválido</translation>
    </message>
    <message>
        <source>Only letters, digits and hyphens are allowed.</source>
        <translation>Apenas letras, dígitos e hífens são permitidos.</translation>
    </message>
    <message>
        <source>Regenerate machine-id</source>
        <translation>Regenerar machine-id</translation>
    </message>
    <message>
        <source>The machine-id identifies this installation (journald, DHCP leases, some licensing).

Regenerating gives the system a new identity — mainly useful after cloning. Continue?</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>Copied</source>
        <translation>Copiado</translation>
    </message>
    <message>
        <source>System information copied to clipboard.</source>
        <translation>Informações do sistema copiadas para a área de transferência.</translation>
    </message>
    <message>
        <source>System Properties</source>
        <translation>Propriedades do Sistema</translation>
    </message>
    <message>
        <source>Computer name</source>
        <translation>Nome do computador</translation>
    </message>
    <message>
        <source>Pretty name</source>
        <translation>Nome amigável</translation>
    </message>
    <message>
        <source>Operating system</source>
        <translation>Sistema operacional</translation>
    </message>
    <message>
        <source>Kernel</source>
        <translation>Kernel</translation>
    </message>
    <message>
        <source>Architecture</source>
        <translation>Arquitetura</translation>
    </message>
    <message>
        <source>Processor</source>
        <translation>Processador</translation>
    </message>
    <message>
        <source>Graphics</source>
        <translation>Gráficos</translation>
    </message>
    <message>
        <source>Installed RAM</source>
        <translation>RAM instalada</translation>
    </message>
    <message>
        <source>Root filesystem</source>
        <translation>Sistema de arquivos raiz</translation>
    </message>
    <message>
        <source>Virtualization</source>
        <translation>Virtualização</translation>
    </message>
    <message>
        <source>none (bare metal)</source>
        <translation>nenhuma (bare metal)</translation>
    </message>
    <message>
        <source>machine-id</source>
        <translation type="unfinished" />
    </message>
    <message>
        <source>System installed</source>
        <translation>Sistema instalado em</translation>
    </message>
    <message>
        <source>Uptime</source>
        <translation>Tempo ligado</translation>
    </message>
</context>
</TS>