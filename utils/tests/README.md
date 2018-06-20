
./utils/tests directory
=======================

## Table of Contents:

1) [What is](#what-is)
2) [Testing script](#testing-script)
3) [Gitlab CI infrastructure](#gitlab-ci-infrastructure)

## What is

In this directory, we store documents related to our testing infrastructure.

## Testing script
To fill.

## Gitlab CI infrastructure

To know more about the Gitlab CI feature, please refer to https://about.gitlab.com/product/continuous-integration/. We use the services offered by Gitlab to test our software in multiple ways. If you are interested how to run these tests on your fork, please read below.

### ns-3 CI configuration

We store our YML files under the directory ./utils/tests. The main file is named `gitlab-ci.yml`, and from there we include multiple files to expand the number (and the quality) of tests we perform. We use inheritance of jobs to avoid to write many lines of code.

### Per commit jobs description

After each commit, the infrastructure will test the grammar correctness by doing a build, with tests and examples enabled, in three modes: debug, release, optimized. The build is done with the default GCC of the Arch Linux distribution: more deep check are done daily and weekly. You can see the job script in `gitlab-ci.yml`. If the build stage is passed, the commits done on the master branch will also trigger a documentation update. Currently, we do not use the generated documentation as Gitlab pages, but we use a separate service to display the documentation through the web.

### Daily jobs description

Thanks to the "Schedule" feature of Gitlab, we setup pipelines that have to be run once per day. The scheduled pipeline has to define a variable, named `RELEASE`, that should be set to `daily`. In the scripts then, we check for the value of that variable and run the jobs accordingly. As daily jobs, we perform a test run in all the modes (debug, release, optimized) under Arch Linux, and they are defined in the file `gitlab-ci-test.yml`.

### Weekly jobs description

As weekly jobs, we perform the build, testing, and documentation stage in every platform we support (Ubuntu, Fedora, Arch Linux) with all the compilers we support (GCC and CLang). To add the support for your platform, please see how the jobs are constructed (for instance, the GCC jobs are in `gitlab-ci-gcc.yml`). We currently miss the jobs for OS X and Windows.
