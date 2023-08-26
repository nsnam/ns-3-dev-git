./utils/tests directory
=======================

## Table of Contents:

1) [What is](#what-is)
2) [Gitlab CI infrastructure](#gitlab-ci-infrastructure)
3) [Test your changes](#test-your-changes)

## What is

In this directory, we store documents related to our testing infrastructure.

## Gitlab CI infrastructure

To know more about the Gitlab CI feature, please refer to https://about.gitlab.com/product/continuous-integration/. We use the services offered by Gitlab to test our software in multiple ways. If you are interested how to run these tests on your fork, please read below.

Note: each user does have a limited amount of "minutes" (time used to run CI jobs). Minutes are limited, and if you run out of minutes you will not be able to run jobs until the next month - or until you pay for more minutes.

### ns-3 CI configuration

We store our YML files under the directory ./utils/tests. The main file is named `gitlab-ci.yml`, and from there we include multiple files to expand the number (and the quality) of tests we perform. We use inheritance of jobs to avoid to write many lines of code.

The configuration is split in different files, each one containing a group of jobs that have some common rules. E.g.:

- `gitlab-ci-per-commit.yml`: jobs executed at each commit.
- `gitlab-ci-scheduled.yml`: daily and weekly ordinary and slow jobs.
- `gitlab-ci-gcc.yml`: jobs to test various GCC versions.

If you did fork ns-3, the CI should already point to the right configureation file. Otherwise, you will have to select the `gitlab-ci.yml` in your fork configuration. Note that Gitlab requires a user verification (usually it requires a valid credit card) to run CI jobs. If you did skip the verification process the CI will always fail.

### CI pipelines

If configured properly, the CI will start a pipeline (a set of jobs) each time new commits are pushed to the repository. To avoid to trigger a pipeline, it is possible to issue the command `git push -o ci.skip`.

Each pipeline is made of several stages, and there are different pipelines depending if there is an active merge request or not.

#### CI pipeline stages

The stages are:

- pre (fast jobs that are easy to fail, like coding style checks and spell checks).
- pre-build (dummy jobs used to trigger following jobs according to specific rules).
- build (build jobs)
- test (test jobs, run the script `test.py` and other checks that require a previous build job)
- code-linting (code quality checks, e.g., clang-tidy)
- documentation (build the docs and check for errors)

Normally, stages are executed sequentially (a stage is stared when the previous is finished), but jobs dependencies might override this.

### Per commit jobs description

After each commit, the infrastructure will test the grammar correctness by doing a build, with tests and examples enabled, in three modes: debug, default, optimized. The build is done with the default GCC of the Arch Linux distribution: more deep check are done daily and weekly. You can see the job script in `gitlab-ci.yml`. If the build stage is passed, the commits done on the master branch will also trigger a documentation update. Currently, we do not use the generated documentation as Gitlab pages, but we use a separate service to display the documentation through the web.

Note that if a commit is pushed to a branch associated to an active merge request, the jobs will be run in a different way, and it will be possible to manually trigger additional jobs. Use this opportunity with caution, as it does consume nsnam minutes (not user minutes).

### Daily jobs description

Thanks to the "Schedule" feature of Gitlab, we setup pipelines that have to be run once per day. The scheduled pipeline has to define a variable, named `RELEASE`, that should be set to `daily`. In the scripts then, we check for the value of that variable and run the jobs accordingly. As daily jobs, we perform a test run in all the modes (debug, default, optimized) under Arch Linux.

### Weekly jobs description

As weekly jobs, we perform the build, testing, and documentation stage in every platform we support (Ubuntu, Fedora, Arch Linux) with all the compilers we support (GCC and CLang). Weekly pipelines should define a variable, named `RELEASE`, as `weekly`. To add the support for your platform, please see how the jobs are constructed (for instance, the GCC jobs are in `gitlab-ci-gcc.yml`). We currently miss the jobs for OS X and Windows.

## Test your changes

When you fork ns-3-dev on Gitlab, you get access to some free hours of execution time on the Gitlab CI infrastructure. You automatically inherit all the scripts as well, so you can test your changes before requesting a merge. As mentioned previously, we store our YML files under the directory ./utils/tests, therefore, to execute per-commit script automatically, Gitlab CI/CD requires a custom path to the YML file.

To customize the path:

1. Go to the project’s **Settings > CI / CD**.
2. Expand the **General pipelines** section.
3. Provide `utils/tests/gitlab-ci.yml` as a value in the **Custom CI configuration path** field.
4. Click **Save changes**.

To perform a deeper test, you can manually run the daily or the weekly test. Go to the Gitlab interface, then enter in the CI/CD menu and select Pipelines. On the top, you can manually run a pipeline: select the branch, and add a variable `RELEASE` set to `daily` or `weekly` following your need, and then run it.

Related to the timeout, our `gitlab-ci.yml` script also configures the job-level timeout. Currently, this timeout is set to 24h considering the extensive and time-consuming testing done in weekly jobs.

**Note**: The job-level timeout can exceed the [project-level](https://docs.gitlab.com/ee/ci/pipelines/settings.html#set-a-limit-for-how-long-jobs-can-run) timeout (default: 60 min), but can not exceed the Runner-specific timeout.


To summarize, if you are unsure about the commit you're about to
merge into master, you can push it to a branch of your fork, then go to
the Gitlab interface, click CI/CD, click Schedules, create a New
schedule, set the description, any pattern that you wish to use, and the
target branch you want to test. Then, put one or more variables from the
list below:

```shell
RELEASE, that can take the value "daily" or "weekly" (if you want to perform all the build/test that are done daily or once a week, respectively)
```

... and then click Save, and run it manually from the "Schedules" page.
