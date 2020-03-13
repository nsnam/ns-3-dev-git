./utils/tests directory
=======================

## Table of Contents:

1) [What is](#what-is)
2) [Testing script](#testing-script)
3) [Gitlab CI infrastructure](#gitlab-ci-infrastructure)
4) [Test your changes](#test-your-changes)

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

As weekly jobs, we perform the build, testing, and documentation stage in every platform we support (Ubuntu, Fedora, Arch Linux) with all the compilers we support (GCC and CLang). Weekly pipelines should define a variable, named `RELEASE`, as `weekly`. To add the support for your platform, please see how the jobs are constructed (for instance, the GCC jobs are in `gitlab-ci-gcc.yml`). We currently miss the jobs for OS X and Windows.

## Test your changes

When you fork ns-3-dev on Gitlab, you get access to 2000 free hours of execution time on the Gitlab CI infrastructure. You automatically inherit all the scripts as well, so you can test your changes before requesting a merge. As mentioned previously, we store our YML files under the directory ./utils/tests, therefore, to execute per-commit script automatically, Gitlab CI/CD requires a custom path to the YML file.

To customize the path:

1. Go to the project’s **Settings > CI / CD**.
2. Expand the **General pipelines** section.
3. Provide `utils/tests/gitlab-ci.yml` as a value in the **Custom CI configuration path** field.
4. Click **Save changes**.

To perform a deeper test, you can manually run the daily or the weekly test. Go to the Gitlab interface, then enter in the CI/CD menu and select Pipelines. On the top, you can manually run a pipeline: select the branch, and add a variable `RELEASE` set to `daily` or `weekly` following your need, and then run it.

Related to the timeout, our `gitlab-ci.yml` script also configures the job-level timeout. Currently, this timeout is set to 9h considering the extensive and time-consuming testing done in weekly jobs.

**Note**: The job-level timeout can exceed the [project-level](https://docs.gitlab.com/ce/user/project/pipelines/settings.html#timeout) timeout (default: 60 min), but can not exceed the Runner-specific timeout.


To summarize, if you are unsure about the commit you're about to
merge into master, you can push it to a branch of your fork, then go to
the Gitlab interface, click CI/CD, click Schedules, create a New
schedule, set the description, any pattern that you wish to use, and the
target branch you want to test. Then, put one or more variables from the
list below:

```shell
RELEASE, that can take the value "weekly" or "daily" or "monthly"
(if you want to perform all the build/test that are done daily, once a week,
or once a month, respectively)

FEDORA = True
(to test all the fedora builds)

UBUNTU = True
(to test all the Ubuntu builds)

GCC_BUILD_ENABLE = True
(to test all the GCC builds)

CLANG_BUILD_ENABLE = True
(to test all the clang builds)
```

... and then click Save, and run it manually from the "Schedules" page.
