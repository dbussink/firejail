# Firejail profile for Mozilla Firefox (Iceweasel in Debian)
noblacklist ${HOME}/.mozilla
include /etc/firejail/disable-mgmt.inc
include /etc/firejail/disable-secret.inc
include /etc/firejail/disable-common.inc
include /etc/firejail/disable-devel.inc
include /etc/firejail/disable-terminals.inc
caps.drop all
seccomp
protocol unix,inet,inet6,netlink
netfilter
tracelog
noroot
whitelist ${DOWNLOADS}
mkdir ~/.mozilla
whitelist ~/.mozilla
mkdir ~/.cache
mkdir ~/.cache/mozilla
mkdir ~/.cache/mozilla/firefox
whitelist ~/.cache/mozilla/firefox
whitelist ~/dwhelper
whitelist ~/.zotero
whitelist ~/.lastpass
whitelist ~/.vimperatorrc
whitelist ~/.vimperator
whitelist ~/.pentadactylrc
whitelist ~/.pentadactyl
whitelist ~/.keysnail.js
whitelist ~/.config/gnome-mplayer
whitelist ~/.cache/gnome-mplayer/plugin
whitelist ~/.pki
include /etc/firejail/whitelist-common.inc

# experimental features
#private-etc passwd,group,hostname,hosts,localtime,nsswitch.conf,resolv.conf,gtk-2.0,pango,fonts,iceweasel,firefox,adobe,mime.types,mailcap,asound.conf,pulse


