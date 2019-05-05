# For remote game servers

# Run as root to perform initial setup and user account creation



echo ""
echo ""
echo "Future root ssh logins will be key-based only, with password forbidden"
echo ""
echo ""


mkdir .ssh

chmod 744 .ssh

cd .ssh

echo "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQDXCsgHA0xvxll5C17p9p7aBQ0lsRCzpG1UhiKviG4ikAAHo0lHd8ywhCumycYJxNHkkWOxNrVkDEb8Ge4lVaJCpGzqkvDth/qhm7MUZZCQIITaYXW89ODGpHMCMBnq6y7FpEBkYQR9Cr4wB8GygUjuE1jhPnL6GRLooV8tLZ0DX4T5rFTfNHfLpkJ+BpoPwDW/rf2RcGO/e1vCuoDQQY/85tCyRs3cRv6yQkRPKyzOESJjhqri6j34e7fzPuVJzKqQlbiuvYoqLweoUbqEd0CwtYIhy9yXCeZSb1ZrIFQ+pNbaoapOgQqe9+g8T0WBOTx1Punwr6AGQWDxM6o5Qse0EeTLbUubOLfddavZueL/yUqQt9gpB64VRBeUZFf7ZmRS4VFi4YbSNvoldurnDu7E0VllXCsw8uCYNHBgyqgqDAmWho887uJ+5dxKsz6ZVH98aLDpUjCJqN5kqBF09F2SeO2AUtvvEabq86frwUhs5pAffpjY6EcyMO+cTK8/i5J2SYCQieFAOH/S1c7kjDvZOAaxRcPFi9r3sT/CO39p7fE5qwgPC7eFRgJAV59WS1bBbd31/QFFSwUgeHN9DAKLtBcWUWCZqCRgZxVYfdosSW/RXhiw/lbIUw+B1t3PfBRaG9WvvJA0HvE4UyWPpYY+YizxrTa/gcqt2aPT479F9w== richard@crucible" > authorized_keys

chmod 644 authorized_keys

sed -i 's/PermitRootLogin yes/PermitRootLogin without-password/' /etc/ssh/sshd_config

service ssh restart


echo ""
echo ""
echo "Installing packages..."
echo ""
echo ""



apt-get -o Acquire::ForceIPv4=true update
apt-get -y install emacs-nox mercurial git g++ expect gdb make fail2ban ufw


echo ""
echo ""
echo "Whitelisting only main server and backup server IP address for ssh"
echo "Opening port 8005 for game server"
echo ""
echo ""

ufw allow from 176.58.100.188 to any port 22
ufw allow 8005
ufw --force enable

echo ""
echo ""
echo "Setting up custom core file name"
echo ""
echo ""

echo "core.%e.%p.%t" > /proc/sys/kernel/core_pattern

echo "" >> /etc/sysctl.conf
echo "" >> /etc/sysctl.conf
echo "# custom core file name" >> /etc/sysctl.conf
echo "kernel.core_pattern=core.%e.%p.%t" >> /etc/sysctl.conf



echo ""
echo ""
echo "Setting up new user account richard without a password..."
echo ""
echo ""

useradd -m -s /bin/bash richard


dataName="OneLifeData7"


su richard<<EOSU

cd /home/richard

echo "ulimit -c unlimited >/dev/null 2>&1" >> ~/.bash_profile

ulimit -c unlimited

mkdir checkout
cd checkout



echo "Using data repository $dataName"

git clone https://github.com/chardbury/crucible-code.git OneLife
git clone https://github.com/chardbury/crucible-data.git $dataName
git clone https://github.com/chardbury/crucible-gems.git minorGems


cd $dataName

lastTaggedDataVersion=\`git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/Crucible_v* | sed -e 's/Crucible_v//'\`


echo "" 
echo "Most recent Data git version is:  \$lastTaggedDataVersion"
echo ""

git checkout -q Crucible_v\$lastTaggedDataVersion



cd ../OneLife/server

echo "http://onehouronelife.com/ticketServer/server.php" > settings/ticketServerURL.ini

ln -s ../../$dataName/objects .
ln -s ../../$dataName/transitions .
ln -s ../../$dataName/categories .
ln -s ../../$dataName/tutorialMaps .
ln -s ../../$dataName/dataVersionNumber.txt .

git for-each-ref --sort=-creatordate --format '%(refname:short)' --count=1 refs/tags/Crucible_v* | sed -e 's/Crucible_v//' > serverCodeVersionNumber.txt


./configure 1

make

bash -l ./runHeadlessServerLinux.sh

echo -n "1" > ~/keepServerRunning.txt

crontab /home/richard/checkout/OneLife/scripts/remoteServerCrontabSource


cd /home/richard
mkdir .ssh

chmod 744 .ssh

cd .ssh

echo "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQDXCsgHA0xvxll5C17p9p7aBQ0lsRCzpG1UhiKviG4ikAAHo0lHd8ywhCumycYJxNHkkWOxNrVkDEb8Ge4lVaJCpGzqkvDth/qhm7MUZZCQIITaYXW89ODGpHMCMBnq6y7FpEBkYQR9Cr4wB8GygUjuE1jhPnL6GRLooV8tLZ0DX4T5rFTfNHfLpkJ+BpoPwDW/rf2RcGO/e1vCuoDQQY/85tCyRs3cRv6yQkRPKyzOESJjhqri6j34e7fzPuVJzKqQlbiuvYoqLweoUbqEd0CwtYIhy9yXCeZSb1ZrIFQ+pNbaoapOgQqe9+g8T0WBOTx1Punwr6AGQWDxM6o5Qse0EeTLbUubOLfddavZueL/yUqQt9gpB64VRBeUZFf7ZmRS4VFi4YbSNvoldurnDu7E0VllXCsw8uCYNHBgyqgqDAmWho887uJ+5dxKsz6ZVH98aLDpUjCJqN5kqBF09F2SeO2AUtvvEabq86frwUhs5pAffpjY6EcyMO+cTK8/i5J2SYCQieFAOH/S1c7kjDvZOAaxRcPFi9r3sT/CO39p7fE5qwgPC7eFRgJAV59WS1bBbd31/QFFSwUgeHN9DAKLtBcWUWCZqCRgZxVYfdosSW/RXhiw/lbIUw+B1t3PfBRaG9WvvJA0HvE4UyWPpYY+YizxrTa/gcqt2aPT479F9w== richard@crucible" > authorized_keys

chmod 644 authorized_keys


exit
EOSU

echo ""
echo "Done with remote setup."
echo ""

