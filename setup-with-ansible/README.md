# README

## How to use

Make a file including IP addresses or host names to be set up. Any file name is OK. Here we name it 'hosts' like the following example.

    $ cat hosts
    [targets]
    192.168.10.1

If you set up the machine on itself, the following can be acceptable.

    [targets]
    localhost

Run:

    $ ansible-playbook -i hosts setup-hatohol-dev.yaml

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
