# README

## How to use

### With localhost or other hosts

Make a file including IP addresses or host names to be set up. Any file name is OK. Here we name it 'hosts' like the following example.

    $ cat hosts
    [targets]
    192.168.10.1

If you set up the machine on itself, the following can be acceptable.

    [targets]
    localhost

Run:

    $ ansible-playbook -i hosts setup-hatohol-dev.yaml

or you can run it w/o ssh

    $ sudo ansible-playbook -c local -i hosts setup-hatohol-dev.yaml

### With Vagrant

Run:

    $ vagrant up hatohol-centos-6-x86_64
    $ vagrant ssh hatohol-centos-6-x86_64

You can destroy the VM by the following command:

    $ vagrant destroy --force hatohol-centos-6-x86_64

You can recreate clean VM after you destroy your VM.

## Troubleshooting

### `Received message too long 1349281121`

Create a file: ansible.cfg with the following content in the directory the command runs on

    [ssh_connection]
    scp_if_ssh=True

### `Please login as the user "ubuntu" rather than the user "root".`

Replace 'user: root' into 'user: ubuntu' and set sudo: True in setup-hatohol-dev.yaml as below.

    user: ubutnu
    sudo: True

This problem happens when the target OS is Ubuntu for cloud.

### `stderr: E: There are problems and -y was used without --force-yes`

It may be solved by doing the following command on the target machine.

    $ sudo apt-get update

### A step `TASK: [upgrade-server/ubuntu | upgrade a server]` never ends

On a machine with Ubuntu 14.04.3 LTS on Openstack, installation of
the kernel image failed due to SEGV of modprobe btrfs. As a result,
upgrade-server process stopped and ansible-playbook stalled.
This can be worked around by adding the following line to
/etc/modprobe.d/blacklist.conf
(you may need to reboot the machine after adding it and before retrying
ansible-playbook)

    blacklist btrfs

