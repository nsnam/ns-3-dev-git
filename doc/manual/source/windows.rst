.. include:: replace.txt
.. highlight:: console

.. Section Separators:
   ----
   ****
   ++++
   ====
   ~~~~

.. _Windows 10:

Windows 10
----------

This chapter describes installation steps specific to Windows 10 and its
derivatives (e.g. Home, Pro, Enterprise) using the Msys2/MinGW64 toolchain.

.. _Windows 10 package prerequisites:

Windows 10 package prerequisites
********************************

The following instructions are relevant to the ns-3.37 release and Windows 10.

Installation of the Msys2 environment
+++++++++++++++++++++++++++++++++++++

.. _Msys2: https://www.msys2.org/

The `Msys2`_ includes ports of Unix tools for Windows built with multiple toolchains,
including: MinGW32, MinGW64, Clang64, UCRT.

The MinGW64 (GCC) toolchain is the one ns-3 was tested.

The `Msys2`_ installer can be found on their site.
Msys2 will be installed by default in the ``C:\msys64`` directory.

The next required step is adding the binaries directories from the MinGW64 toolchain and generic Msys2 tools
to the PATH environment variable. This can be accomplished via the GUI (search for system environment variable),
or via the following command (assuming it was installed to the default directory):

.. sourcecode:: console

    C:\> setx PATH "C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%;" /m

Note: if the MinGW64 binary directory doesn't precede the Windows/System32 directory (already in ``%PATH%``),
the documentation build will fail since Windows has a conflicting ``convert`` command (FAT-to-NTFS). Similarly,
the Msys64 binary directory doesn't precede the Windows/System2 directory, running the ``bash`` command will
result in Windows trying to run the Windows Subsystem for Linux (WSL) bash shell.

Accessing the MinGW64 shell
+++++++++++++++++++++++++++

After installing Msys2 and adding the binary directories to the ``PATH``, we can access the Unix-like MinGW64 shell
and use the Pacman package manager.

The Pacman package manager is similar to the one used by Arch Linux, and can be accessed via one of the Msys2 shells.
In this case, we will be using the MinGW64 shell. We can take this opportunity to update the package cache and packages.

.. sourcecode:: console

    C:\ns-3-dev\> set MSYSTEM MINGW64
    C:\ns-3-dev\> bash
    /c/ns-3-dev/ MINGW64$ pacman -Syu

Pacman will request you to close the shell and re-open it to proceed after the upgrade.


Minimal requirements for C++ (release)
++++++++++++++++++++++++++++++++++++++

This is the minimal set of packages needed to run ns-3 C++ programs from a released tarball.

.. sourcecode:: console

    /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-ninja mingw-w64-x86_64-grep mingw-w64-x86_64-sed mingw-w64-x86_64-python


Netanim animator
++++++++++++++++

Qt5 development tools are needed for Netanim animator. Qt4 will also work but we have migrated to qt5.

.. sourcecode:: console

    /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-qt5 git


Support for MPI-based distributed emulation
+++++++++++++++++++++++++++++++++++++++++++

The MPI setup requires two parts.

The first part is the Microsoft MPI SDK required to build the MPI applications,
which is distributed via Msys2.

.. sourcecode:: console

    /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-msmpi

.. _Microsoft MPI: https://www.microsoft.com/en-us/download/details.aspx?id=100593

The second part is the `Microsoft MPI`_ executors (mpiexec, mpirun) package,
which is distributed as an installable (``msmpisetup.exe``).

After installing it, the path containing the executors also need to be included to ``PATH``
environment variable of the Windows and/or the MinGW64 shell, depending on whether
you want to run MPI programs in either shell or both of them.

.. sourcecode:: console

    C:\ns-3-dev\> setx PATH "%PATH%;C:\Program Files\Microsoft MPI\Bin" /m
    C:\ns-3-dev\> set MSYSTEM MINGW64
    C:\ns-3-dev\> bash
    /c/ns-3-dev/ MINGW64$ echo "export PATH=$PATH:/c/Program\ Files/Microsoft\ MPI/Bin" >> ~/.bashrc

Debugging
+++++++++

GDB is installed along with the mingw-w64-x86_64-toolchain package.

Support for utils/check-style.py code style check program
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. sourcecode:: console

    /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-uncrustify


Doxygen and related inline documentation
++++++++++++++++++++++++++++++++++++++++

To build the Doxygen-based documentation, we need doxygen and a Latex toolchain.
Getting Doxygen from Msys2 is straightforward.

.. sourcecode:: console

  /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-imagemagick mingw-w64-x86_64-doxygen mingw-w64-x86_64-graphviz

For unknown reasons, the texlive packages in Msys2 are broken,
so we recommend manually installing the official Texlive.

.. _Texlive for Windows: https://tug.org/texlive/windows.html

Start by installing `Texlive for Windows`_. It can be installed either via the network installer or
directly from the ISO with all optional packages.

To install it with the network installer, start by creating a texlive folder and downloading the
texlive configuration profile below. You can change the installation directory (starting with
``C:/texlive/2022``).

.. sourcecode:: raw

    selected_scheme scheme-custom
    TEXDIR C:/texlive/2022
    TEXMFCONFIG ~/.texlive2022/texmf-config
    TEXMFHOME ~/texmf
    TEXMFLOCAL C:/texlive/texmf-local
    TEXMFSYSCONFIG C:/texlive/2022/texmf-config
    TEXMFSYSVAR C:/texlive/2022/texmf-var
    TEXMFVAR ~/.texlive2022/texmf-var
    binary_win32 1
    collection-basic 1
    collection-bibtexextra 1
    collection-binextra 1
    collection-context 1
    collection-fontsrecommended 1
    collection-fontutils 1
    collection-games 1
    collection-humanities 1
    collection-langenglish 1
    collection-latex 1
    collection-latexextra 1
    collection-latexrecommended 1
    collection-luatex 1
    collection-mathscience 1
    collection-metapost 1
    collection-music 1
    collection-pictures 1
    collection-plaingeneric 1
    collection-pstricks 1
    collection-publishers 1
    collection-texworks 1
    collection-wintools 1
    collection-xetex 1
    instopt_adjustpath 1
    instopt_adjustrepo 1
    instopt_letter 0
    instopt_portable 0
    instopt_write18_restricted 1
    tlpdbopt_autobackup 1
    tlpdbopt_backupdir tlpkg/backups
    tlpdbopt_create_formats 1
    tlpdbopt_desktop_integration 1
    tlpdbopt_file_assocs 1
    tlpdbopt_generate_updmap 0
    tlpdbopt_install_docfiles 0
    tlpdbopt_install_srcfiles 0
    tlpdbopt_post_code 1
    tlpdbopt_sys_bin /usr/local/bin
    tlpdbopt_sys_info /usr/local/share/info
    tlpdbopt_sys_man /usr/local/share/man
    tlpdbopt_w32_multi_user 0


Then, download Texlive's web installer and unpack it to the texlive directory,
and run the installer.

.. sourcecode:: console

    ~$ wget https://linorg.usp.br/CTAN/systems/texlive/tlnet/install-tl.zip
    ~$ python3 -c "import shutil; shutil.unpack_archive('install-tl.zip', './')"
    ~$ cd install-tl-*
    ~/install-tl-20220923$ .\install-tl-windows.bat --no-gui --lang en -profile ..\texlive.profile

After the installation finishes, add the installation directory to the PATH environment variable.

.. sourcecode:: console

    setx PATH "%PATH%;C:\texlive\2022\bin\win32" /m
    C:\ns-3-dev\> set MSYSTEM MINGW64
    C:\ns-3-dev\> bash
    /c/ns-3-dev/ MINGW64$ echo "export PATH=$PATH:/c/texlive/2022/bin/win32" >> ~/.bashrc

The ns-3 manual, models and tutorial
++++++++++++++++++++++++++++++++++++

These documents are written in reStructuredText for Sphinx (doc/tutorial, doc/manual, doc/models).
The figures are typically written in dia.

The documents can be generated into multiple formats, one of them being pdf,
which requires the same Latex setup for doxygen.

Sphinx can be installed via Msys2's package manager:

.. sourcecode:: console

  /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-python-sphinx

.. _Dia: https://sourceforge.net/projects/dia-installer/

`Dia`_ on the other hand needs to be downloaded and installed manually.
After installing it, or unzipping the package (example below), add Dia/bin to the PATH.

.. sourcecode:: console

    C:\ns-3-dev\> setx PATH "%PATH%;C:\dia_0.97.2_win32\bin" /m
    C:\ns-3-dev\> set MSYSTEM MINGW64
    C:\ns-3-dev\> bash
    /c/ns-3-dev/ MINGW64$ echo "export PATH=$PATH:/c/dia_0.97.2_win32/bin" >> ~/.bashrc

GNU Scientific Library (GSL)
++++++++++++++++++++++++++++

GSL is used to provide more accurate WiFi error models and can be installed with:

.. sourcecode:: console

  /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-gsl

Database support for statistics framework
+++++++++++++++++++++++++++++++++++++++++

SQLite3 is installed along with the mingw-w64-x86_64-toolchain package.


Xml-based version of the config store
+++++++++++++++++++++++++++++++++++++

Requires libxml2 >= version 2.7, which can be installed with the following.

.. sourcecode:: console

  /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-libxml2

Support for openflow module
+++++++++++++++++++++++++++

Requires some boost libraries that can be installed with the following.

.. sourcecode:: console

  /c/ns-3-dev/ MINGW64$ pacman -S mingw-w64-x86_64-boost

Windows 10 Docker container
***************************

Docker containers are not as useful for Windows, since only Windows hosts can use them,
however we add directions on how to use the Windows container and how to update the Docker
image for reference.

First, gather all dependencies previously mentioned to cover all supported features.
Install them to a base directory for the container (e.g. ``C:\tools``).

Save the following Dockerfile to the base directory.

.. sourcecode:: docker

    # It is really unfortunate we need a 16 GB base image just to get the installers working, but such is life
    FROM mcr.microsoft.com/windows:20H2

    # Copy the current host directory to the container
    COPY .\\ C:/tools
    WORKDIR C:\\tools

    # Create temporary dir
    RUN mkdir C:\\tools\\temp

    # Export environment variables
    RUN setx PATH "C:\\tools\\msys64\\mingw64\\bin;C:\\tools\\msys64\\usr\\bin;%PATH%" /m
    RUN setx PATH "%PATH%;C:\\Program Files\\Microsoft MPI\\bin;C:\\tools\\dia\bin;C:\tools\texlive\2022\bin\win32" /m
    RUN setx MSYSTEM "MINGW64" /m

    # Install Msys2
    RUN .\\msys2-x86_64-20220503.exe in --confirm-command --accept-messages --root C:\\tools\\msys64

    # Update base packages
    RUN C:\\tools\\msys64\\usr\\bin\\pacman -Syyuu --noconfirm

    # Install base packages
    RUN bash -c "echo export PATH=$PATH:/c/Program\ Files/Microsoft\ MPI/Bin >> /c/tools/msys64/home/$USER/.bashrc" && \
        bash -c "echo export PATH=$PATH:/c/tools/dia/bin >> /c/tools/msys64/home/$USER/.bashrc" && \
        bash -c "echo export PATH=$PATH:/c/tools/texlive/2022/bin/win32 >> /c/tools/msys64/home/$USER/.bashrc" && \
        bash -c "pacman -S mingw-w64-x86_64-toolchain \
                       mingw-w64-x86_64-cmake \
                       mingw-w64-x86_64-ninja \
                       mingw-w64-x86_64-grep \
                       mingw-w64-x86_64-sed \
                       mingw-w64-x86_64-qt5 \
                       git \
                       mingw-w64-x86_64-msmpi \
                       mingw-w64-x86_64-uncrustify \
                       mingw-w64-x86_64-imagemagick \
                       mingw-w64-x86_64-doxygen \
                       mingw-w64-x86_64-graphviz \
                       mingw-w64-x86_64-python-sphinx \
                       mingw-w64-x86_64-gsl \
                       mingw-w64-x86_64-libxml2 \
                       mingw-w64-x86_64-boost \
                       --noconfirm"

    # Install Microsoft MPI
    RUN .\\msmpisetup.exe -unattend -force -full -verbose

    # Install TexLive
    RUN .\\install-tl-20220526\\install-tl-windows.bat --no-gui --lang en -profile .\\texlive.profile

    # Move working directory to temp and start cmd
    WORKDIR C:\\tools\\temp
    ENTRYPOINT ["cmd"]

Now you should be able to run ``docker build -t username/image .``.

After building the container image, you should be able to use it:

.. sourcecode:: console

    $ docker run -it username/image
    C:\tools\temp$ git clone https://gitlab.com/nsnam/ns-3-dev
    C:\tools\temp$ cd ns-3-dev
    C:\tools\temp\ns-3-dev$ python ns3 configure --enable-tests --enable-examples
    C:\tools\temp\ns-3-dev$ python ns3 build
    C:\tools\temp\ns-3-dev$ python test.py

If testing succeeds, the container image can then be pushed to the Docker Hub using
``docker push username/image``.


Windows 10 Vagrant
******************

.. _VirtualBox: https://www.virtualbox.org/
.. _Vagrant: https://www.vagrantup.com/
.. _virtual machine: https://www.redhat.com/en/topics/virtualization/what-is-a-virtual-machine
.. _boxes available: https://app.vagrantup.com/boxes/search

As an alternative to manually setting up all dependencies required by ns-3,
one can use a pre-packaged `virtual machine`_. There are many ways to do that,
but for automation, the most used certainly is `Vagrant`_.

Vagrant supports multiple virtual machine providers, is available in all platforms and is
fairly straightforward to use and configure.

There are many `boxes available`_ offering guests operating systems such as BSD, Mac, Linux and Windows.

Using the pre-packaged Vagrant box
++++++++++++++++++++++++++++++++++

The provider for the ns-3 Vagrant box is `VirtualBox`_.

The reference Windows virtual machine can be downloaded via the following Vagrant command

.. sourcecode:: console

    ~/mingw64_test $ vagrant init gabrielcarvfer/ns3_win10_mingw64

After that, a Vagrantfile will be created in the current directory (in this case, mingw64_test).

The file can be modified to adjust the number of processors and memory available to the virtual machine (VM).

.. sourcecode:: console

    ~/mingw64_test $ cat Vagrantfile
    # -*- mode: ruby -*-
    # vi: set ft=ruby :

    # All Vagrant configuration is done below. The "2" in Vagrant.configure
    # configures the configuration version (we support older styles for
    # backwards compatibility). Please don't change it unless you know what
    # you're doing.
    Vagrant.configure("2") do |config|
    # The most common configuration options are documented and commented below.
    # For a complete reference, please see the online documentation at
    # https://docs.vagrantup.com.

    # Every Vagrant development environment requires a box. You can search for
    # boxes at https://vagrantcloud.com/search.
    config.vm.box = "gabrielcarvfer/ns3_win10_mingw64"

    # Disable automatic box update checking. If you disable this, then
    # boxes will only be checked for updates when the user runs
    # `vagrant box outdated`. This is not recommended.
    # config.vm.box_check_update = false

    # Create a forwarded port mapping which allows access to a specific port
    # within the machine from a port on the host machine. In the example below,
    # accessing "localhost:8080" will access port 80 on the guest machine.
    # NOTE: This will enable public access to the opened port
    # config.vm.network "forwarded_port", guest: 80, host: 8080

    # Create a forwarded port mapping which allows access to a specific port
    # within the machine from a port on the host machine and only allow access
    # via 127.0.0.1 to disable public access
    # config.vm.network "forwarded_port", guest: 80, host: 8080, host_ip: "127.0.0.1"

    # Create a private network, which allows host-only access to the machine
    # using a specific IP.
    # config.vm.network "private_network", ip: "192.168.33.10"

    # Create a public network, which generally matched to bridged network.
    # Bridged networks make the machine appear as another physical device on
    # your network.
    # config.vm.network "public_network"

    # Share an additional folder to the guest VM. The first argument is
    # the path on the host to the actual folder. The second argument is
    # the path on the guest to mount the folder. And the optional third
    # argument is a set of non-required options.
    # config.vm.synced_folder "../data", "/vagrant_data"

    # Provider-specific configuration so you can fine-tune various
    # backing providers for Vagrant. These expose provider-specific options.
    # Example for VirtualBox:
    #
    # config.vm.provider "virtualbox" do |vb|
    #   # Display the VirtualBox GUI when booting the machine
    #   vb.gui = true
    #
    #   # Customize the amount of memory on the VM:
    #   vb.memory = "1024"
    # end
    #
    # View the documentation for the provider you are using for more
    # information on available options.

    # Enable provisioning with a shell script. Additional provisioners such as
    # Ansible, Chef, Docker, Puppet and Salt are also available. Please see the
    # documentation for more information about their specific syntax and use.
    # config.vm.provision "shell", inline: <<-SHELL
    #   apt-get update
    #   apt-get install -y apache2
    # SHELL
    end

We can uncomment the virtualbox provider block and change vCPUs and RAM.
It is recommended never to match the number of vCPUs to the number of thread of the machine,
or the host operating system can become unresponsive.
For compilation workloads, it is recommended to allocate 1-2 GB of RAM per vCPU.

.. _user Vagrantfile:

.. sourcecode:: console

    ~/mingw64_test/ $ cat Vagrantfile
    # -*- mode: ruby -*-
    # vi: set ft=ruby :
    Vagrant.configure("2") do |config|
    config.vm.box = "gabrielcarvfer/ns3_win10_mingw64"
      config.vm.provider "virtualbox" do |vb|
        vb.cpus = "8"
        vb.memory = "8096" # 8GB of RAM
      end
    end

After changing the settings, we can start the VM and login via ssh. The default password is "vagrant".

.. sourcecode:: console

    ~/mingw64_test/ $ vagrant up
    ~/mingw64_test/ $ vagrant ssh
    C:\Users\vagrant>


We are now logged into the machine and ready to work. If you prefer to update the tools, get into the
MinGW64 shell and run pacman.

.. sourcecode:: console

    C:\Users\vagrant\> set MSYSTEM MINGW64
    C:\Users\vagrant\> bash
    /c/Users/vagrant/ MINGW64$ pacman -Syu
    /c/Users/vagrant/ MINGW64$ exit
    C:\Users\vagrant\>

At this point, we can clone ns-3 locally:

.. sourcecode:: console

    C:\Users\vagrant> git clone `https://gitlab.com/nsnam/ns-3-dev`
    C:\Users\vagrant> cd ns-3-dev
    C:\Users\vagrant\ns-3-dev> python3 ns3 configure --enable-tests --enable-examples --enable-mpi
    C:\Users\vagrant\ns-3-dev> python3 test.py

We can also access the ~/mingw64_test/ from the host machine, where the Vagrantfile resides, by accessing
the synchronized folder C:\vagrant.
If the Vagrantfile is in the host ns-3-dev directory, we can continue working on it.

.. sourcecode:: console

    C:\Users\vagrant> cd C:\vagrant
    C:\vagrant\> python3 ns3 configure --enable-tests --enable-examples --enable-mpi
    C:\vagrant\> python3 test.py

If all the PATH variables were set for the MinGW64 shell, we can also use it instead of the
default CMD shell.

.. sourcecode:: console

    C:\vagrant\> set MSYSTEM=MINGW64
    C:\vagrant\> bash
    /c/vagrant/ MINGW64$ ./ns3 clean
    /c/vagrant/ MINGW64$ ./ns3 configure --enable-tests --enable-examples --enable-mpi
    /c/vagrant/ MINGW64$ ./test.py

To stop the Vagrant machine, we should close the SSH session then halt.

.. sourcecode:: console

    /c/vagrant/ MINGW64$ exit
    C:\vagrant\> exit
    ~/mingw64_test/ vagrant halt

To destroy the machine (e.g. to restore the default settings), use the following.

.. sourcecode:: console

    vagrant destroy

Packaging a new Vagrant box
+++++++++++++++++++++++++++

**BEWARE: DO NOT CHANGE THE SETTINGS MENTIONED ON A REAL MACHINE**

**THE SETTINGS ARE MEANT FOR A DISPOSABLE VIRTUAL MACHINE**

.. _Windows 10 ISO: https://www.microsoft.com/pt-br/software-download/windows10ISO

Start by downloading the `Windows 10 ISO`_.

Then install `VirtualBox`_.

Configure a VirtualBox VM and use the Windows 10 ISO file as the install source.

During the installation, create a local user named "vagrant" and set its password to "vagrant".

Check for any Windows updates and install them.

The following commands assume administrative permissions and a PowerShell shell.

Install the VirtualBox guest extensions
=======================================

On the VirtualBox GUI, click on ``Devices->Insert Guest Additions CD Image...``
to download the VirtualBox guest extensions ISO and mount it as a CD drive on the guest VM.

Run the installer to enable USB-passthrough, folder syncing and others.

After installing, unmount the drive by removing it from the VM. Click on ``Settings->Storage``,
select the guest drive and remove it clicking the button with an red ``x``.

Install the OpenSSH server
==========================

Open PowerShell and run the following to install OpenSSH server,
then set it to start automatically and open the firewall ports.

.. sourcecode:: console

    Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0
    Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
    Start-Service sshd
    Set-Service -Name sshd -StartupType 'Automatic'
    if (!(Get-NetFirewallRule -Name "OpenSSH-Server-In-TCP" -ErrorAction SilentlyContinue | Select-Object Name, Enabled)) {
        Write-Output "Firewall Rule 'OpenSSH-Server-In-TCP' does not exist, creating it..."
        New-NetFirewallRule -Name 'OpenSSH-Server-In-TCP' -DisplayName 'OpenSSH Server (sshd)' -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22
    } else {
        Write-Output "Firewall rule 'OpenSSH-Server-In-TCP' has been created and exists."
    }

Enable essential services and disable unnecessary ones
======================================================

Ensure the following services are set to **automatic** from the Services panel(services.msc):

    - Base Filtering Engine
    - Remote Procedure Call (RPC)
    - DCOM Server Process Launcher
    - RPC Endpoint Mapper
    - Windows Firewall

Ensure the following services are set to **disabled** from the Services panel(services.msc):

    - Windows Update
    - Windows Update Remediation
    - Windows Search

The same can be accomplished via the command-line with the following commands:

.. sourcecode:: console

    Set-Service -Name BFE -StartupType 'Automatic'
    Set-Service -Name RpcSs -StartupType 'Automatic'
    Set-Service -Name DcomLaunch -StartupType 'Automatic'
    Set-Service -Name RpcEptMapper -StartupType 'Automatic'
    Set-Service -Name mpssvc -StartupType 'Automatic'
    Set-Service -Name wuauserv -StartupType 'Disabled'
    Set-Service -Name WaaSMedicSvc -StartupType 'Disabled'
    Set-Service -Name WSearch -StartupType 'Disabled'

Install the packages you need
=============================

In this step we install all the software required by ns-3, as listed in the Section `Windows 10 package prerequisites`_.

Disable Windows Defender
========================

After installing everything, it should be safe to disable the Windows security.

Enter in the Windows Security settings and disable "anti-tamper protection".
It rollbacks changes to security settings periodically.

Enter in the Group Policy Editor (gpedit.msc) and disable:

    - Realtime protection
    - Behavior monitoring
    - Scanning of archives, removable drives, network files, scripts
    - Windows defender


The same can be accomplished with the following command-line commands.

.. sourcecode:: console

    Set-MpPreference -DisableArchiveScanning 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableBehaviorMonitoring 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableIntrusionPreventionSystem 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableIOAVProtection 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableRemovableDriveScanning 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableBlockAtFirstSeen 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableScanningMappedNetworkDrivesForFullScan 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableScanningNetworkFiles 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableScriptScanning 1 -ErrorAction SilentlyContinue
    Set-MpPreference -DisableRealtimeMonitoring 1 -ErrorAction SilentlyContinue
    Set-Service -Name WdNisSvc -StartupType 'Disabled'
    Set-Service -Name WinDefend -StartupType 'Disabled'
    Set-Service -Name Sense -StartupType 'Disabled'
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows Defender\Real-Time Protection" -Name SpyNetReporting -Value 0
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows Defender\Real-Time Protection" -Name SubmitSamplesConsent -Value 0
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows Defender\Features" -Name TamperProtection -Value 4
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows Defender" -Name DisableAntiSpyware -Value 1
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows Defender" -Name DisableAntiSpyware -Value 1

Note: the previous commands were an excerpt from the complete script in:
https://github.com/jeremybeaume/tools/blob/master/disable-defender.ps1

Turn off UAC notifications
==========================

The UAC notifications are the popups where you can give your OK to elevation to administrative privileges.
It can be disabled via User Account Control Settings, or via the following commands.

.. sourcecode:: console

    reg ADD HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System /v EnableLUA /t REG_DWORD /d 0 /f


Change the strong password security Policy
==========================================

Open the Local Security Policy management window. Under Security Settings/Account Policy/Password Policy,
disable the option saying "Password must meet complexity requirements".


Testing
=======

After you reach this point, reboot your machine then log back in.

Test if all your packages are working as expected.

In the case of ns-3, try to enable all supported features, run the test.py and test-ns3.py suites.

If everything works, then try to log in via SSH.

If everything works, shut down the machine and prepare for packaging.

The network interface configured should be a NAT. Other interfaces won't work correctly.

Default Vagrantfile
===================

Vagrant can package VirtualBox VMs into Vagrant boxes without much more work.
However, it still needs one more file to do that: the default Vagrantfile.

This file will be used by Vagrant to configure the VM later on and how to connect to it.

.. sourcecode:: ruby

    # -*- mode: ruby -*-
    # vi: set ft=ruby :

    Vagrant.configure("2") do |config|
    config.vm.box = "BOX_FILE.box" # name of the box
    config.vm.communicator = "winssh" # indicate that we are talking to a windows box via ssh
    config.vm.guest = :windows # indicate that the guest is a windows machine
    config.vm.network :forwarded_port, guest: 3389, host: 3389, id: "rdp", auto_correct: true
    config.ssh.password = "vagrant" # give the default password, so that it stops trying to use a .ssh key-pair
    config.ssh.insert_key = false # let the user use a written password
    config.ssh.keys_only = false
    config.winssh.shell = "cmd" # select the default shell (could be cmd or powershell)
      config.vm.provider :virtualbox do |v, override|
          #v.gui = true # do not show the VirtualBox GUI if unset or set to false
          v.customize ["modifyvm", :id, "--memory", 8096] # the default settings for the VM are 8GB of RAM
          v.customize ["modifyvm", :id, "--cpus", 8] # the default settings for the VM are 8 vCPUs
          v.customize ["modifyvm", :id, "--vram", 128] # 128 MB or vGPU RAM
          v.customize ["modifyvm", :id, "--clipboard", "bidirectional"]
          v.customize ["setextradata", "global", "GUI/SuppressMessages", "all" ]
      end
    end

This Vagrantfile will be baked into the Vagrant box, and can be modified by the `user Vagrantfile`_.
After writing the Vagrantfile, we can call the following command.

.. sourcecode:: console

    vagrant package --vagrantfile Vagrantfile --base VIRTUALBOX_VM_NAME --output BOX_FILE.box

It will take an awful long time depending on your drive.

After it finishes, we can add the box to test it.

.. sourcecode:: console

    vagrant box add BOX_NAME BOX_FILE
    vagrant up BOX_NAME
    vagrant ssh

If it can connect to the box, you are ready to upload it to the Vagrant servers.

Publishing the Vagrant box
==========================

Create an account in https://app.vagrantup.com/ or log in with yours.
In the dashboard, you can create a new box named BOX_NAME or select an existing one to update.

After you select your box, click to add a provider. Pick Virtualbox.
Calculate the MD5 hash of your BOX_FILE.box and fill the field then click to proceed.

Upload the box.

Now you should be able to download your box from the Vagrant servers via the the following command.

.. sourcecode:: console

    vagrant init yourUserName/BOX_NAME
    vagrant up
    vagrant ssh

More information on Windows packaging to Vagrant boxes is available here:

    - https://www.vagrantup.com/docs/boxes/base
    - https://www.vagrantup.com/docs/vagrantfile/machine_settings
    - https://www.vagrantup.com/docs/vagrantfile/ssh_settings
    - https://www.vagrantup.com/docs/vagrantfile/winssh_settings
    - https://github.com/pghalliday/windows-vagrant-boxes
