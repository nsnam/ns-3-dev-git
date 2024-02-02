.. include:: replace.txt
.. highlight:: bash


.. _Docker : https://docs.docker.com/desktop/
.. _rootless mode : https://docs.docker.com/engine/security/rootless/
.. _Podman : https://podman.io/
.. _Apptainer : https://apptainer.org/docs/user/latest/
.. _continuous integration (CI) : https://docs.gitlab.com/ee/ci/

.. _Working with Docker:

Working with Docker
-------------------

The ns-3 project repository is currently hosted in GitLab, which includes
`continuous integration (CI)`_ tools to automate build, tests, packaging and
distribution of software. The CI works based on jobs, that are defined
in YAML files, which specify a container the job will run on.

See :ref:`Working with gitlab-ci-local<Working with gitlab-ci-local>` for how
to use containers for running our CI checks locally.

A container is a lightweight virtualization tool that allows one to use user space
tools from different OSes on top of the same kernel. This drastically cuts the
overhead of alternatives such as full virtualization, where a complete operating
system and the hardware it runs on needs to be emulated, or the hardware switched
between a host and a guest OS via a hypervisor.

The most common type of containers are applications containers, where
:ref:`Docker <Docker containers>` is the most popular solution.

.. _Docker pricing: https://www.docker.com/pricing/

Note on pricing: notice that commercial and governmental use of Docker
may require a subscription. See `Docker pricing`_ for more information.
:ref:`Podman <Podman containers>` is an open-source drop-in replacement.

Note on security: Docker is installed by default with root privileges.
This is a security hazard if you are running untrusted software,
especially in shared environments. Docker can be installed in
`rootless mode`_ for better isolation, however, the alternatives such
as `Podman`_ and `Apptainer`_ (previously Singularity) provide safer
default settings. You can read more on `Docker security`_.

.. _Docker containers:

Docker containers
*****************

`Docker`_ popularized containers and made them ubiquitous in continuous integration
due to its ease of use. This section will consolidate the basics of how to work
with Docker or its compatible alternatives.

.. _Docker security: https://docs.docker.com/engine/security/

Docker workflow typically consists of 10 steps:

#. `Install Docker`_
#. `Write a Dockerfile`_
#. `Build a Docker container image`_
#. `Create a container based on the image`_
#. `Start the container`_
#. `Access the container`_
#. `Stop the container`_
#. `Deleting the container`_
#. `Publishing the container image`_
#. `Deleting the container image`_

.. _Install Docker:

Install Docker
==============

Docker is usually set up (*e.g.* via system package managers or their official
installers) in root mode, requiring frequent use of administrative
permissions/sudo, which is necessary for some services, but not for most.
For proper isolation, install Docker in `rootless mode`_, or use one of
its compatible alternatives.

.. _Write a Dockerfile:

Write a Dockerfile
==================

.. _special keywords: https://docs.docker.com/engine/reference/builder/

A Dockerfile is a file that contains instructions of how to build an image of the
container that will host the application / service. These files are composed of
one-line commands using `special keywords`_. The most common keywords are:

* ``FROM``: used to specify a parent image
* ``COPY``: used to copy files from the external build directory into the container
* ``RUN``: run shell commands to install dependencies and set up the service
* ``ENTRYPOINT``: program to be started when the container is launched

Here is one example of a ``Dockerfile`` that can be used to debug a ns-3
program on Fedora 37:

.. sourcecode:: docker

    FROM fedora:37

    RUN dnf update --assumeyes && dnf install --assumeyes gcc-c++ cmake ccache ninja-build python gdb

    ENTRYPOINT ["bash"]

When Docker is called to build the container image for this Dockerfile, it will
pull the image for fedora:37 from a ``Docker Registry``. Common registries
include docker.io (which hosts images shown in the DockerHub webpage), quay.io,
GitHub, GitLab, or you can have your own self-hosted option.

Note: For security reasons, it is not recommended to run third-party images
in production. Dockerfiles are embedded into docker images and can be checked.
One example of how this can be done is shown below:

.. sourcecode:: console

    $ docker image ls
    REPOSITORY                    TAG         IMAGE ID      CREATED       SIZE
    fedoradebugging               37          d96491e23a80  10 days ago   829 MB

    $ docker history fedoradebugging
    ID            CREATED       CREATED BY                                     SIZE        COMMENT
    0b56b184852b  10 days ago   /bin/sh -c #(nop) ENTRYPOINT ["bash"]          0 B
    <missing>     10 days ago   /bin/sh -c dnf update --assumeyes && dnf i...  648 MB      FROM registry.fedoraproject.org/fedora:37
    4105b568d464  2 months ago                                                 182 MB      Created by Image Factory

We can see different ``layers`` (commands) that compose the final docker image.
When compared to the Dockerfile, they show up in reverse order from the latest
to the earliest event.

.. _Build a Docker container image:

Build a Docker container image
==============================

When building toolchain containers to work on projects, we typically don't
want to copy the projects themselves into the container. So we need to pass
an empty directory to Docker.

We also need to specify a tag for our image, otherwise it will have a randomly
assigned name which we will have to refer to later.

.. sourcecode:: console

    $ mkdir empty-dir

    $ docker image ls
    REPOSITORY                         TAG         IMAGE ID      CREATED             SIZE

    $ docker build -t fedoradebugging:37 -f Dockerfile ./empty-dir
    STEP 1/3: FROM fedora:37
    Resolved "fedora" as an alias (/etc/containers/registries.conf.d/shortnames.conf)
    Trying to pull registry.fedoraproject.org/fedora:37...
    Getting image source signatures
    Copying blob 80b613d8f1ff done
    Copying config 4105b568d4 done
    Writing manifest to image destination
    Storing signatures
    STEP 2/3: RUN dnf update --assumeyes && dnf install --assumeyes gcc-c++ cmake ccache ninja-build python gdb
    Fedora 37 - x86_64                              4.0 MB/s |  82 MB     00:20
    Fedora 37 openh264 (From Cisco) - x86_64        2.1 kB/s | 2.5 kB     00:01
    Fedora Modular 37 - x86_64                      1.6 MB/s | 3.8 MB     00:02
    Fedora 37 - x86_64 - Updates                    5.0 MB/s |  41 MB     00:08
    Fedora Modular 37 - x86_64 - Updates            1.3 MB/s | 2.9 MB     00:02
    Last metadata expiration check: 0:00:01 ago on Fri Feb  2 13:41:30 2024.
    Dependencies resolved.
    ================================================================================
     Package                         Arch       Version           Repository   Size
    ================================================================================
    Upgrading:
     elfutils-default-yama-scope     noarch     0.190-2.fc37      updates      12 k
     ...
    (38/56): cmake-3.27.7-1.fc37.x86_64.rpm         1.8 MB/s | 7.8 MB     00:04
    (39/56): cpp-12.3.1-1.fc37.x86_64.rpm           1.8 MB/s |  11 MB     00:05
    ...
    Complete!
    --> c33f842f2fc
    STEP 3/3: ENTRYPOINT ["bash"]
    COMMIT fedoradebugging:37
    --> 2412e124d12
    Successfully tagged localhost/fedoradebugging:37
    2412e124d1257914a70fde70289948f7880fdbd1d3b213c206ff691995ecc03e

    $ docker image ls
    REPOSITORY                         TAG         IMAGE ID      CREATED             SIZE
    localhost/fedoradebugging          37          2412e124d125  About a minute ago  829 MB
    registry.fedoraproject.org/fedora  37          4105b568d464  2 months ago        182 MB

Notice that our image got the repository name prepended (``localhost``).
This is a Podman thing, with the objective people specify the repositories
their images come from, instead of risking pulling an image from different
registries and risk pulling an infected image.

.. _Create a container based on the image:

Create a container based on the image
=====================================

Note: For toolchain containers, which is the typical use case
for ns-3 testing, we use the ``run`` command instead. It creates
and starts the container in a single command. You can jump directly
to the `intended way to use toolchain containers`_, or continue
reading in case you want to learn a bit more about Docker.

Now that we have our container image, we can create a container
based on it. It is like booting a brand-new computer with a freshly
installed operating system and commonly used programs.

Docker containers can be created with the following:

.. sourcecode:: console

    $ docker container ls -a
    CONTAINER ID  IMAGE       COMMAND     CREATED     STATUS      PORTS       NAMES

    $ docker container create --name fedoradeb fedoradebugging:37
    8cb9edea0dcfe4a2335dd5b73766a6985275f9ee3e0b6fd5ea5b03eedbd4bbb1

    $ docker container ls -a
    CONTAINER ID  IMAGE                         COMMAND     CREATED         STATUS      PORTS       NAMES
    8cb9edea0dcf  localhost/fedoradebugging:37              13 seconds ago  Created                 fedoradeb

.. _Start the container:

Start the container
===================

Containers can be started with the following commands:

.. sourcecode:: console

    $ docker start fedoradeb
    fedoradeb

    $ docker container ls -a
    CONTAINER ID  IMAGE                         COMMAND     CREATED             STATUS                     PORTS       NAMES
    8cb9edea0dcf  localhost/fedoradebugging:37              About a minute ago  Exited (0) 13 seconds ago              fedoradeb

As it can be seen, our container was started, then executed bash, but it
was in a non-interactive session, so it doesn't wait for the user. The
process created by the entrypoint then exited, and the container
manager stopped the container.

This is useful for containers running services, like web servers,
but not useful at all to use it as a toolchain container. In the
next section, we see the preferred way to start a container toolchain.

.. _Access the container:

Access the container
====================

For service containers, after starting the container with
``start``, we can execute commands on the running container
using ``exec``. We are going to use a different container for
demonstration purposes.

.. sourcecode:: console

    $ docker pull nginx:latest
    Resolving "nginx" using unqualified-search registries (/etc/containers/registries.conf)
    Trying to pull docker.io/library/nginx:latest...
    Getting image source signatures
    Copying blob 398157bc5c51 done
    Copying blob f0bd99a47d4a done
    Copying blob f24a6f652778 done
    Copying blob c57ee5000d61 done
    Copying blob 9f3589a5fc50 done
    Copying blob 9b0163235c08 done
    Copying blob 1ef1c1a36ec2 done
    Copying config b690f5f0a2 done
    Writing manifest to image destination
    Storing signatures
    b690f5f0a2d535cee5e08631aa508fef339c43bb91d5b1f7d77a1a05cea021a8

    $ docker container create --name nginx nginx:latest
    b6fb5a96bed7552a8164ee20e20cb2dd39ac98f6a7c7c076d0e22b93bc85b988

    $ docker container ls -a
    CONTAINER ID  IMAGE                           COMMAND               CREATED         STATUS      PORTS       NAMES
    b6fb5a96bed7  docker.io/library/nginx:latest  nginx -g daemon o...  16 seconds ago  Created                 nginx

    $ docker start nginx
    nginx

    $ docker exec nginx whereis nginx
    nginx: /usr/sbin/nginx /usr/lib/nginx /etc/nginx /usr/share/nginx

    $ docker exec -it nginx bash

    root@b6fb5a96bed7:/# echo "Printing a message in the container"
    Printing a message in the container

    root@b6fb5a96bed7:/# exit
    exit

    $ docker container stop nginx
    nginx

    $ docker container ls -a
    CONTAINER ID  IMAGE                           COMMAND               CREATED        STATUS                    PORTS       NAMES
    b6fb5a96bed7  docker.io/library/nginx:latest  nginx -g daemon o...  5 minutes ago  Exited (0) 7 seconds ago              nginx

    $ docker container rm nginx
    b6fb5a96bed7552a8164ee20e20cb2dd39ac98f6a7c7c076d0e22b93bc85b988

    $ docker container ls -a
    CONTAINER ID  IMAGE                           COMMAND               CREATED        STATUS                    PORTS       NAMES

    $ docker image ls
    REPOSITORY                         TAG         IMAGE ID      CREATED         SIZE
    docker.io/library/nginx            latest      b690f5f0a2d5  3 months ago    191 MB

    $ docker image rm nginx:latest
    Untagged: docker.io/library/nginx:latest
    Deleted: b690f5f0a2d535cee5e08631aa508fef339c43bb91d5b1f7d77a1a05cea021a8

.. _intended way to use toolchain containers:

The intended way to access toolchain containers
###############################################

Other than ``start/stop/exec``, Docker also has a ``run`` option, that builds a
brand-new container from the container image for each time it is executed.
This is the intended way to use toolchain containers.

.. sourcecode:: console

    $ docker run -it fedoradebugging:37

    [root@6fa29fa742c5 /]# echo "Printing a message in the container"
    Printing a message in the container

    [root@6fa29fa742c5 /]# exit
    exit

    $

Now we need to access files that are not inside the container volume. To do
this, we can mount an external volume as a local directory.

.. sourcecode:: console

    $ mkdir -p ./external/ns-3-dev

    $ echo "hello" > ./external/ns-3-dev/msg.txt

    $ docker run -it -v ./external/ns-3-dev:/internal/ns-3-dev fedoradebugging:37

    [root@6fa29fa742c5 /]# cat /internal/ns-3-dev/msg.txt
    hello

    [root@8f0fd2eded04 /]# exit
    exit

    $

With this, we can point to the real ns-3-dev directory and work as usual.

.. sourcecode:: console

    $ docker run -it -v ./ns-3-dev:/ns-3-dev fedoradebugging:37

    [root@067a8b748816 /]# cd ns-3-dev/

    [root@067a8b748816 ns-3-dev]# ./ns3 configure
    Warn about uninitialized values.
    -- The CXX compiler identification is GNU 12.3.1
    ...
    -- ---- Summary of ns-3 settings:
    Build profile                 : default
    Build directory               : /ns-3-dev/build
    Build with runtime asserts    : ON
    Build with runtime logging    : ON
    Build version embedding       : OFF (not requested)
    ...
    -- Configuring done (26.5s)
    -- Generating done (11.6s)
    -- Build files have been written to: /ns-3-dev/cmake-cache
    Finished executing the following commands:
    /usr/bin/cmake3 -S /ns-3-dev -B /ns-3-dev/cmake-cache -DCMAKE_BUILD_TYPE=default -DNS3_ASSERT=ON -DNS3_LOG=ON -DNS3_WARNINGS_AS_ERRORS=OFF -DNS3_NATIVE_OPTIMIZATIONS=OFF -G Ninja -
    -warn-uninitialized

The container will be automatically stopped after exiting.

.. sourcecode:: console

    [root@067a8b748816 ns-3-dev]# exit
    exit

    $ docker container ls
    CONTAINER ID  IMAGE       COMMAND     CREATED     STATUS      PORTS       NAMES


.. _Stop the container:

Stop the container
==================

For long-running, non-interactive containers, use:

.. sourcecode:: console

    $ docker container stop containername


.. _Deleting the container:

Deleting the container
======================

To delete a container, it must be stopped first.
If it isn't listed in ``docker container ls``, that
is already the case.

.. sourcecode:: console

    $ docker container ls -a
    CONTAINER ID  IMAGE                         COMMAND     CREATED        STATUS                      PORTS       NAMES
    067a8b748816  localhost/fedoradebugging:37              3 minutes ago  Exited (0) 5 seconds ago                relaxed_carver

    $ docker container rm relaxed_carver
    067a8b748816715d93ede9099132d943c04b13fcc23b3b8a7e21d23c1096cc2a

.. _Publishing the container image:

Publishing the container image
==============================

To publish an image, you need to tag it prepending the
Docker registry you are submitting your image.

For example, to submit our ``localhost/fedoradebugging:37``
to the Docker.io registry, we would add the
``docker.io/username/fedoradebugging:37``.

.. sourcecode:: console

    $ docker tag localhost/fedoradebugging:37 docker.io/username/fedoradebugging:37

    $ docker push docker.io/username/fedoradebugging:37

If you don't want to push your image to a public registry, you can
run your own registry inside a container. The following command
will set up a registry container, and expose it to port 5000 of
the host device. This allows third parties to access the service
hosted inside the container, as long as the host firewall allows it.

.. sourcecode:: console

    $ docker run -d -p 5000:5000 --restart always --name registry registry:2
    Resolved "registry" as an alias (/etc/containers/registries.conf.d/shortnames.conf)
    Trying to pull docker.io/library/registry:2...
    Getting image source signatures
    Copying blob 619be1103602 done
    Copying blob d1a4f6454cb2 done
    Copying blob 0da701e3b4d6 done
    Copying blob 2ba4b87859f5 done
    Copying blob 14a4d5d702c7 done
    Copying config a8781fe3b7 done
    Writing manifest to image destination
    Storing signatures
    fa61d0ba582bc4953d81163abdb174dea937c15f5882a5395a857dfa8b8fc4fc

    $ docker tag fedoradebugging:37 localhost:5000/fedoradebugging:37

    $ docker push localhost:5000/fedoradebugging:37


Note: you can also publish your container images to GitLab and GitHub
container registries. You first need to login into the registries,
then set the appropriate tag for the registry and push the image.

Here is an example for GitLab:

.. sourcecode:: console

    $ docker login -u user_name registry_url

    $ docker tag fedoradebugging:37 registry.gitlab.com/user_name/project_name/fedoradebugging:37

    $ docker push registry.gitlab.com/user_name/project_name/fedoradebugging:37
    Getting image source signatures
    Copying blob cb6b836430b4 [================================>-----] 152.0MiB / 173.3MiB
    Copying blob cb6b836430b4 [================================>-----] 152.0MiB / 173.3MiB
    Copying blob cb6b836430b4 [================================>-----] 152.0MiB / 173.3MiB
    Copying blob cb6b836430b4 [================================>-----] 152.0MiB / 173.3MiB
    Copying blob cb6b836430b4 [================================>-----] 152.0MiB / 173.3MiB
    Getting image source signatures
    Copying blob cb6b836430b4 done
    Copying config 4105b568d4 done
    Writing manifest to image destination
    Storing signatures

    $ docker image prune -a
    WARNING! This command removes all images without at least one container associated with them.
    Are you sure you want to continue? [y/N] y
    4105b568d464732d3ea6a3ce9a0095f334fa7aa86fdd1129f288d2687801d87d

    $ docker image ls
    REPOSITORY  TAG         IMAGE ID    CREATED     SIZE

    $ docker pull registry.gitlab.com/user_name/project_name/fedoradebugging:37
    Trying to pull registry.gitlab.com/user_name/project_name/fedoradebugging:37...
    Getting image source signatures
    Copying blob 6eb8dda2c1ca done
    Copying config 4105b568d4 done
    Writing manifest to image destination
    Storing signatures
    4105b568d464732d3ea6a3ce9a0095f334fa7aa86fdd1129f288d2687801d87d


We can also build our docker images directly from the GitLab CI and publish them
with the following jobs:

.. sourcecode:: yaml

    .build-and-register-docker-image:
      image: docker
      services:
        - docker:dind
      before_script:
        - mkdir -p $HOME/.docker
        - docker login -u $CI_REGISTRY_USER -p $CI_JOB_TOKEN $CI_REGISTRY
      script:
        - docker build --pull -f $DOCKERFILE -t $CI_REGISTRY_IMAGE/$DOCKERTAG .
        - docker push $CI_REGISTRY_IMAGE/$DOCKERTAG

    fedoradebugging37:
      extends:
        - .build-and-register-docker-image
      variables:
        DOCKERFILE: "Dockerfile.fedoradebugging37.txt"
        DOCKERTAG: "fedoradebugging:37"

.. _Deleting the container image:

Deleting the container image
============================

Same command we have shown a few times in the previous command.

.. sourcecode:: console

    $ docker image ls
    REPOSITORY                         TAG         IMAGE ID      CREATED            SIZE
    localhost/fedoradebugging          37          2412e124d125  About an hour ago  829 MB
    localhost:5000/fedoradebugging     37          2412e124d125  About an hour ago  829 MB

    $ docker image rm localhost:5000/fedoradebugging:37

    $ docker image ls
    REPOSITORY                         TAG         IMAGE ID      CREATED            SIZE
    localhost/fedoradebugging          37          2412e124d125  About an hour ago  829 MB

    $ docker image rm localhost/fedoradebugging:37
    Untagged: localhost/fedoradebugging:37
    Deleted: 2412e124d1257914a70fde70289948f7880fdbd1d3b213c206ff691995ecc03e
    Deleted: c33f842f2fc0181b3b5f1d71456b0ffb8a2ec94e8d093f4873c61c492dd7e3dd

.. _Podman containers:

Podman containers
*****************

`Podman`_ is the drop-in replacement for Docker made by RedHat.
You can install it and set up an alias using the following command.

.. sourcecode:: console

    echo alias docker=podman >> ~/.bashrc

To get a docker-like experience, you might want to
also change a few settings, such as search repositories for unqualified
image names (without the prepended repository/registry).

This can be done by adding the following line in ``/etc/containers/registries.conf``

.. sourcecode:: text

    unqualified-search-registries = ["registry.fedoraproject.org", "registry.access.redhat.com", "docker.io"]


