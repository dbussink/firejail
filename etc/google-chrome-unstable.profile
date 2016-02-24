# Google Chrome unstable browser profile
noblacklist ${HOME}/.config/google-chrome-unstable
include /etc/firejail/disable-mgmt.inc
include /etc/firejail/disable-secret.inc
include /etc/firejail/disable-common.inc
include /etc/firejail/disable-terminals.inc

# chromium is distributed with a perl script on Arch
# include /etc/firejail/disable-devel.inc
#

netfilter
whitelist ${DOWNLOADS}
mkdir ~/.config
mkdir ~/.config/google-chrome-unstable
whitelist ~/.config/google-chrome-unstable
mkdir ~/.cache
mkdir ~/.cache/google-chrome-unstable
whitelist ~/.cache/google-chrome-unstable
mkdir ~/.pki
whitelist ~/.pki
include /etc/firejail/whitelist-common.inc

