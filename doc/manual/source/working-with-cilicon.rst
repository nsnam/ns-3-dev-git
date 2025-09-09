.. include:: replace.txt
.. highlight:: bash

.. _Working with Cilicon:

Working with Cilicon
--------------------

.. _continuous integration (CI) : https://docs.gitlab.com/ee/ci/
.. _Cilicon : https://github.com/traderepublic/Cilicon/
.. _Cilicon releases : https://github.com/traderepublic/Cilicon/releases/

The ns-3 project repository is currently hosted in GitLab, which includes
`continuous integration (CI)`_ tools to automate build, tests, packaging and
distribution of software. The CI works based on jobs, that are defined
in YAML files and run inside containers.

While we can run most Linux containers everywhere without much problem,
that is not the case for Windows and MacOS. You can run gitlab-runner as a
local user, taking proper precautions to avoid that user from escalating privileges,
then taking over your own network and other systems on it.

That is typically harder than just enforce network security, and virtualization-based security.
GitLab doesn't offer a way to do that, other than their own runners, which cost up to 6x more
than linux ones.
Which is why Trade Republic created the `Cilicon`_ application for MacOS, which creates an ephemeral virtual
machine, executes a gitlab job, then destroys the VM, and creates a fresh new one.

To set it up, first go to System Settings->Energy and enable all options to prevent sleep, wake on
network traffic, and restart after power failure (assuming the Mac will be used exclusively as a CI server).

After this, download the latest release in `Cilicon releases`_.
Put the application in Applications directory and start it. It will ask you to install MacOS virtualization framework.

If you only have a single CI runner that you wish to run, follow their easy settings and create a
``~/cilicon.yml`` file at the user directory containing:

.. sourcecode:: yaml

    source: oci://ghcr.io/cirruslabs/macos-runner:sequoia
    provisioner:
      type: gitlab
      config:
        gitlabURL: <GITLAB_INSTANCE_URL>
        runnerToken: <RUNNER_TOKEN>

If you have multiple CI runners that you wish to run from a single machine, one job at a time,
you need a custom configuration file, first create a folder that will be shared with the VM.

.. sourcecode:: console

    mkdir /Users/nsnam/gitlab-ephemeral

Then add your GitLab runner ``config.toml`` file there, for example, containing:

.. sourcecode:: toml

    concurrent = 1
    check_interval = 0
    connection_max_age = "15m0s"
    shutdown_timeout = 0

    [session_server]
      session_timeout = 1800

    [[runners]]
      name = "MacOsRunnerForProjectA"
      url = "https://gitlab.com/"
      token = "<RUNNER_TOKEN_A>"
      executor = "shell"
      cleanup_policy = "always"

    [[runners]]
      name = "MacOsRunnerForProjectB"
      url = "https://gitlab.com/"
      token = "<RUNNER_TOKEN_B>"
      executor = "shell"
      cleanup_policy = "always"


Then create a ``cilicon.yml`` file at the user directory containing:

.. sourcecode:: yaml

    source: oci://ghcr.io/cirruslabs/macos-sonoma-xcode:15
    provisioner:
      type: script
      config:
        run: |
          cp /Volumes/My\ Shared\ Files/gitlab-runner/config.toml /Users/admin/.gitlab-runner/config.toml
          gitlab-runner run
    consoleDevices:
      - tart-version-2
    hardware:
      ramGigabytes: 20
      connectsToAudioDevice: false
    directoryMounts:
      - hostPath: /Users/nsnam/gitlab-ephemeral
        guestFolder: gitlab-runner
        readOnly: true

Note: If you want to create a new VM for every CI job, replace ``gitlab-runner run``
with ``gitlab-runner run-single``. Not very recommended, because it will hit SSD hard.
And recent Macs have soldered storage.

After configuring everything, execute Cilicon. It will setup MacOS virtualization framework, then fetch the VM image.
It will then start the VM.

At this point, MacOS will query you for being visible in network.
You need to say yes, so the VM can access the shared directory.

Otherwise, you need to either write the TOML with the cilicon.yml, or download it from a remote server.

At this point, you should have your native MacOS runners fully working.
Try starting a job and see if they respond via GitLab web interface.
