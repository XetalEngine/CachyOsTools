#!/usr/bin/env bash
# Install only in a disposable test source, with its signed kernel archive in
# /opt/xetal-test/. The VM runner waits for these markers and a clean poweroff.
set -Eeuo pipefail
exec > /dev/ttyS0 2>&1
test -s /etc/xetal-test-marker
test -s /etc/xetal-boot.conf
test -d /boot/xetal
mkdir -p /var/lib/xetal-test
if [[ ! -f /var/lib/xetal-test/transaction-done ]]; then
    echo XETAL_RESTORE_BOOT_OK
    pacman --noconfirm -U /opt/xetal-test/*.pkg.tar.zst 2>&1 | tee /var/log/xetal-test-transaction.log
    grep -q "Updating the restored system's boot files" /var/log/xetal-test-transaction.log
    touch /var/lib/xetal-test/transaction-done
    sync
    echo XETAL_KERNEL_TRANSACTION_OK
else
    echo XETAL_UPDATED_BOOT_OK
fi
sync
systemctl poweroff
