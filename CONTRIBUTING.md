# Contributing to ns-3

[comment]: # (This file is heavily inspired by Atom's CONTRIBUTING.md file: https://raw.githubusercontent.com/atom/atom/master/CONTRIBUTING.md)

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

The following is a set of guidelines for contributing to ns-3, which are hosted in the [nsnam organization](https://gitlab.com/nsnam) on GitLab.com. These are mostly guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a merge request.

#### Table Of Contents

[I don't want to read this whole thing, I just have a question!!!](#i-dont-want-to-read-this-whole-thing-i-just-have-a-question)

[What should I know before I get started?](#what-should-i-know-before-i-get-started)
  * [Atom and Packages](#atom-and-packages)
  * [Atom Design Decisions](#design-decisions)

[How Can I Contribute?](#how-can-i-contribute)
  * [Reporting Bugs](#reporting-bugs)
  * [Suggesting Enhancements](#suggesting-enhancements)
  * [Your First Code Contribution](#your-first-code-contribution)
  * [Pull Requests](#pull-requests)

[Styleguides](#styleguides)
  * [Git Commit Messages](#git-commit-messages)
  * [JavaScript Styleguide](#javascript-styleguide)
  * [CoffeeScript Styleguide](#coffeescript-styleguide)
  * [Specs Styleguide](#specs-styleguide)
  * [Documentation Styleguide](#documentation-styleguide)

[Additional Notes](#additional-notes)
  * [Issue and Pull Request Labels](#issue-and-pull-request-labels)


## I don't want to read this whole thing I just have a question!!!

> **Note:** Please don't file an issue to ask a question.
            You'll get faster results by using the resources below.

We have an official message board where the community chimes in with helpful advice if you have questions.

* [ns-3-users, the official ns-3 message board](https://groups.google.com/forum/#!forum/ns-3-users)
* [ns-3 manual](https://www.nsnam.org/docs/manual/html/index.html)

If chat is more your speed, you can join the ns-3 Zulip channel:

* [Join the ns-3 Zulip chat](https://link)
    * Even though Zulip is a chat service, sometimes it takes several hours
      for community members to respond &mdash; please be patient!
    * Use the `#ns3` channel for general questions or discussion about ns-3
    * Use the `#electron` channel for questions about Electron
    * Use the `#packages` channel for questions or discussion about writing or contributing to Atom packages (both core and community)
    * Use the `#ui` channel for questions and discussion about Atom UI and themes
    * There are many other channels available, check the channel list

## What should I know before I get started?

### ns-3 Documentation

The ns-3 project maintains the documentation in different places, dependings on the need. The documentation that is current with the development tree is the following:

* [Tutorial](https://www.nsnam.org/docs/tutorial/html/index.html): Its purpose is to get you started, with simple examples, into the ns-3 world.
* [Manual](https://www.nsnam.org/docs/manual/html/index.html) is a descriptive documentation of the ns-3 core capabilities, as well as shared development practices.
* [Model library](https://www.nsnam.org/docs/models/html/index.html), contains a list of manuals, one for each module we officially ship.
* [Doxygen](https://www.nsnam.org/docs/doxygen/index.html), contains the programming interface of ns-3, as well as description of classes, methods, and functions that permit the interaction between different modules, as well as with the user code.

### ns-3 modules

ns-3 is an open source project &mdash; it's made up of [49 modules](https://gitlab.com/nsnam/ns-3-dev/tree/master/src). When you initially consider contributing to ns-3, you might be unsure about which of those 49 modules implements the functionality you want to change or report a bug for. This section should help you with that.

ns-3 is intentionally very modular, and it's possible that a feature you need or a technology you may want to use is not coming from the official repository, but rather from a [community maintained module](https://apps.nsnam.org/). Each community module has its own repository too, and it should contain the information on how to integrate that code with ns-3. If you are looking, instead, more information on how to work with ns-3's official modules, please take a look first to the [model documentation](https://www.nsnam.org/docs/models/html/index.html).

### Design Decisions

When we make a significant decision in how we maintain the project and what we can or cannot support, we will document it through the [ns-developers mailing list](https://mailman.isi.edu/mailman/listinfo/ns-developers). If you have a question around how we do things, please subscribe and lurk around to see how we proceed. If you have a development problem, please open a new topic and ask your question.

## How Can I Contribute?

### Reporting Bugs

This section guides you through submitting a bug report for ns-3. Following these guidelines helps maintainers and the community understand your report :pencil:, reproduce the behavior :computer: :computer:, and find related reports :mag_right:.

Before creating bug reports, please check [this list](#before-submitting-a-bug-report) as you might find out that you don't need to create one. When you are creating a bug report, please [include as many details as possible](#how-do-i-submit-a-good-bug-report). Fill out [the required template](ISSUE_TEMPLATE.md), the information it asks for helps us resolve issues faster.

> **Note:** If you find a **Closed** issue that seems like it is the same thing that you're experiencing, open a new issue and include a link to the original issue in the body of your new one.

#### Before Submitting A Bug Report (to be completed)

* **Check the [debugging guide](https://flight-manual.atom.io/hacking-atom/sections/debugging/).** You might be able to find the cause of the problem and fix things yourself. Most importantly, check if you can reproduce the problem [in the latest version of Atom](https://flight-manual.atom.io/hacking-atom/sections/debugging/#update-to-the-latest-version), if the problem happens when you run Atom in [safe mode](https://flight-manual.atom.io/hacking-atom/sections/debugging/#check-if-the-problem-shows-up-in-safe-mode), and if you can get the desired behavior by changing [Atom's or packages' config settings](https://flight-manual.atom.io/hacking-atom/sections/debugging/#check-atom-and-package-settings).
* **Check the [FAQs on the forum](https://discuss.atom.io/c/faq)** for a list of common questions and problems.
* **Determine [which repository the problem should be reported in](#atom-and-packages)**.
* **Perform a [cursory search](https://github.com/search?q=+is%3Aissue+user%3Aatom)** to see if the problem has already been reported. If it has **and the issue is still open**, add a comment to the existing issue instead of opening a new one.

#### How Do I Submit A (Good) Bug Report?

Bugs are tracked as [GitLab issues](https://docs.gitlab.com/ee/user/project/issues/). After you've determined [which module](#ns-3-modules) your bug is related to, if it is inside the official distribution then create an issue, label it with the name of the module, and provide the following information by filling in [the template](doc/ISSUE_TEMPLATE.md).

Explain the problem and include additional details to help maintainers reproduce the problem:

* **Use a clear and descriptive title** for the issue to identify the problem.
* **Describe the exact steps which reproduce the problem** in as many details as possible. For example, start by explaining how you coded your script, e.g. which functions exactly you called. When listing steps, **don't just say what you did, but explain how you did it**. For example, if your program includes modifications to the ns-3 core or a module, please list them (or even better, provide the diffs).
* **Provide specific examples to demonstrate the steps**. Include links to files or projects, or copy/pasteable snippets, which you use in those examples. If you're providing snippets in the issue, use [Markdown code blocks](https://docs.gitlab.com/ee/user/markdown.html).
* **Describe the behavior you observed after following the steps** and point out what exactly is the problem with that behavior.
* **Explain which behavior you expected to see instead and why.**
* **If you're reporting that ns-3 crashed**, include a crash report with a stack trace from the operating system. On macOS, the crash report will be available in `Console.app` under "Diagnostic and usage information" > "User diagnostic reports". Include the crash report in the issue in a [code block](https://docs.gitlab.com/ee/user/markdown.html), or a file attachment.
* **If the problem is related to performance or memory**, include a [CPU profile capture](https://flight-manual.atom.io/hacking-atom/sections/debugging/#diagnose-runtime-performance) with your report.

Include details about your configuration and environment:

* **Which version of ns-3 are you using?**
* **What's the name and version of the OS you're using**?
* **Which [modules](#ns-3-modules) do you have installed?**

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for ns-3, including completely new features and minor improvements to existing functionality. Following these guidelines helps maintainers and the community understand your suggestion :pencil: and find related suggestions :mag_right:.

Before creating enhancement suggestions, please check [this list](#before-submitting-an-enhancement-suggestion) as you might find out that you don't need to create one. When you are creating an enhancement suggestion, please [include as many details as possible](#how-do-i-submit-a-good-enhancement-suggestion). Fill in [the template](doc/ISSUE_TEMPLATE.md), including the steps that you imagine you would take if the feature you're requesting existed.

#### Before Submitting An Enhancement Suggestion

* **Check if there's already [a merge request](https://gitlab.com/nsnam/ns-3-dev/merge_requests) which provides that enhancement.**
* **Determine [which module the enhancement should be suggested in](#ns-3-modules).**
* **Perform a [cursory search](https://gitlab.com/nsnam/ns-3-dev/issues)** to see if the enhancement has already been suggested. If it has, add a comment to the existing issue instead of opening a new one.

#### How Do I Submit A (Good) Enhancement Suggestion?

Enhancement suggestions are tracked as [Gitlab issues](https://docs.gitlab.com/ee/user/project/issues/). After you've determined [which module](#ns-3-modules) your enhancement suggestion is related to, create an issue on that repository and provide the following information:

* **Use a clear and descriptive title** for the issue to identify the suggestion.
* **Provide a step-by-step description of the suggested enhancement** in as many details as possible.
* **Provide specific examples to demonstrate the steps**. Include copy/pasteable snippets which you use in those examples.
* **Describe the current behavior** and **explain which behavior you expected to see instead** and why.
* **Explain why this enhancement would be useful** to most ns-3 users.
* **Specify the name and version of the OS you're using.**

### Your First Code Contribution

Unsure where to begin contributing to Atom? You can start by looking through these `beginner` and `help-wanted` issues:

* [Beginner issues][beginner] - issues which should only require a few lines of code, and a test or two.
* [Help wanted issues][help-wanted] - issues which should be a bit more involved than `beginner` issues.

Both issue lists are sorted by total number of comments. While not perfect, number of comments is a reasonable proxy for impact a given change will have.

#### Local development

ns-3 and all packages can be developed locally. For instructions on how to do this, see the following sections in the [ns-3 Manual](https://www.nsnam.org/docs/manual/html/index.html):

* Contributing to ns-3 with git

### Merge Requests

The process described here has several goals:

- Maintain ns-3's quality
- Fix problems that are important to users
- Engage the community in working toward the best possible ns-3
- Enable a sustainable system for ns-3's maintainers to review contributions

Please follow these steps to have your contribution considered by the maintainers:

1. Follow all instructions in [the template](doc/MERGE_REQUEST_TEMPLATE.md)
2. Follow the [styleguides](#styleguides)
3. After you submit your merge request, verify that all status checks are passing <details><summary>What if the status checks are failing?</summary>If a status check is failing, and you believe that the failure is unrelated to your change, please leave a comment on the merge request explaining why you believe the failure is unrelated. A maintainer will re-run the status check for you. If we conclude that the failure was a false positive, then we will open an issue to track that problem with our status check suite.</details>

While the prerequisites above must be satisfied prior to having your merge request reviewed, the reviewer(s) may ask you to complete additional design work, tests, or other changes before your pull request can be ultimately accepted.

## Styleguides

### Git Commit Messages

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues in the first line, by prepending `[#issue]`

### C++ guidelines

We maintain our code style through the webpage [Code Style](https://www.nsnam.org/develop/contributing-code/coding-style/) and as a `clang-format` configure [file](https://gitlab.com/nsnam/ns-3-dev/blob/master/.clang-format)

### Documentation Styleguide

* Use [Doxygen](http://www.doxygen.nl/) for what regards the code.
* Use [ReStructuredText](http://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html) for what regards the model (module) documentation.

## Additional Notes

### Issue and Pull Request Labels

This section lists the labels we use to help us track and manage issues and pull requests.

The labels are loosely grouped by their purpose, but it's not required that every issue have a label from every group or that an issue can't have more than one label from the same group.

Please open an issue on `infrastructure` if you have suggestions for new labels, and if you notice some labels are missing.

#### Type of Issue and Issue State

| Label name | `atom/atom` :mag_right: | `atom`‑org :mag_right: | Description |
| --- | --- | --- | --- |
| `enhancement` | [search][search-atom-repo-label-enhancement] | [search][search-atom-org-label-enhancement] | Feature requests. |
| `bug` | [search][search-atom-repo-label-bug] | [search][search-atom-org-label-bug] | Confirmed bugs or reports that are very likely to be bugs. |
| `question` | [search][search-atom-repo-label-question] | [search][search-atom-org-label-question] | Questions more than bug reports or feature requests (e.g. how do I do X). |
| `feedback` | [search][search-atom-repo-label-feedback] | [search][search-atom-org-label-feedback] | General feedback more than bug reports or feature requests. |
| `help-wanted` | [search][search-atom-repo-label-help-wanted] | [search][search-atom-org-label-help-wanted] | The Atom core team would appreciate help from the community in resolving these issues. |
| `beginner` | [search][search-atom-repo-label-beginner] | [search][search-atom-org-label-beginner] | Less complex issues which would be good first issues to work on for users who want to contribute to Atom. |
| `more-information-needed` | [search][search-atom-repo-label-more-information-needed] | [search][search-atom-org-label-more-information-needed] | More information needs to be collected about these problems or feature requests (e.g. steps to reproduce). |
| `needs-reproduction` | [search][search-atom-repo-label-needs-reproduction] | [search][search-atom-org-label-needs-reproduction] | Likely bugs, but haven't been reliably reproduced. |
| `blocked` | [search][search-atom-repo-label-blocked] | [search][search-atom-org-label-blocked] | Issues blocked on other issues. |
| `duplicate` | [search][search-atom-repo-label-duplicate] | [search][search-atom-org-label-duplicate] | Issues which are duplicates of other issues, i.e. they have been reported before. |
| `wontfix` | [search][search-atom-repo-label-wontfix] | [search][search-atom-org-label-wontfix] | The Atom core team has decided not to fix these issues for now, either because they're working as intended or for some other reason. |
| `invalid` | [search][search-atom-repo-label-invalid] | [search][search-atom-org-label-invalid] | Issues which aren't valid (e.g. user errors). |
| `package-idea` | [search][search-atom-repo-label-package-idea] | [search][search-atom-org-label-package-idea] | Feature request which might be good candidates for new packages, instead of extending Atom or core Atom packages. |
| `wrong-repo` | [search][search-atom-repo-label-wrong-repo] | [search][search-atom-org-label-wrong-repo] | Issues reported on the wrong repository (e.g. a bug related to the [Settings View package](https://github.com/atom/settings-view) was reported on [Atom core](https://github.com/atom/atom)). |

#### Topic Categories

| Label name | `atom/atom` :mag_right: | `atom`‑org :mag_right: | Description |
| --- | --- | --- | --- |
| `windows` | [search][search-atom-repo-label-windows] | [search][search-atom-org-label-windows] | Related to Atom running on Windows. |
| `linux` | [search][search-atom-repo-label-linux] | [search][search-atom-org-label-linux] | Related to Atom running on Linux. |
| `mac` | [search][search-atom-repo-label-mac] | [search][search-atom-org-label-mac] | Related to Atom running on macOS. |
| `documentation` | [search][search-atom-repo-label-documentation] | [search][search-atom-org-label-documentation] | Related to any type of documentation (e.g. [API documentation](https://atom.io/docs/api/latest/) and the [flight manual](https://flight-manual.atom.io/)). |
| `performance` | [search][search-atom-repo-label-performance] | [search][search-atom-org-label-performance] | Related to performance. |
| `security` | [search][search-atom-repo-label-security] | [search][search-atom-org-label-security] | Related to security. |
| `ui` | [search][search-atom-repo-label-ui] | [search][search-atom-org-label-ui] | Related to visual design. |
| `api` | [search][search-atom-repo-label-api] | [search][search-atom-org-label-api] | Related to Atom's public APIs. |
| `uncaught-exception` | [search][search-atom-repo-label-uncaught-exception] | [search][search-atom-org-label-uncaught-exception] | Issues about uncaught exceptions, normally created from the [Notifications package](https://github.com/atom/notifications). |
| `crash` | [search][search-atom-repo-label-crash] | [search][search-atom-org-label-crash] | Reports of Atom completely crashing. |
| `auto-indent` | [search][search-atom-repo-label-auto-indent] | [search][search-atom-org-label-auto-indent] | Related to auto-indenting text. |
| `encoding` | [search][search-atom-repo-label-encoding] | [search][search-atom-org-label-encoding] | Related to character encoding. |
| `network` | [search][search-atom-repo-label-network] | [search][search-atom-org-label-network] | Related to network problems or working with remote files (e.g. on network drives). |
| `git` | [search][search-atom-repo-label-git] | [search][search-atom-org-label-git] | Related to Git functionality (e.g. problems with gitignore files or with showing the correct file status). |

#### `atom/atom` Topic Categories

| Label name | `atom/atom` :mag_right: | `atom`‑org :mag_right: | Description |
| --- | --- | --- | --- |
| `editor-rendering` | [search][search-atom-repo-label-editor-rendering] | [search][search-atom-org-label-editor-rendering] | Related to language-independent aspects of rendering text (e.g. scrolling, soft wrap, and font rendering). |
| `build-error` | [search][search-atom-repo-label-build-error] | [search][search-atom-org-label-build-error] | Related to problems with building Atom from source. |
| `error-from-pathwatcher` | [search][search-atom-repo-label-error-from-pathwatcher] | [search][search-atom-org-label-error-from-pathwatcher] | Related to errors thrown by the [pathwatcher library](https://github.com/atom/node-pathwatcher). |
| `error-from-save` | [search][search-atom-repo-label-error-from-save] | [search][search-atom-org-label-error-from-save] | Related to errors thrown when saving files. |
| `error-from-open` | [search][search-atom-repo-label-error-from-open] | [search][search-atom-org-label-error-from-open] | Related to errors thrown when opening files. |
| `installer` | [search][search-atom-repo-label-installer] | [search][search-atom-org-label-installer] | Related to the Atom installers for different OSes. |
| `auto-updater` | [search][search-atom-repo-label-auto-updater] | [search][search-atom-org-label-auto-updater] | Related to the auto-updater for different OSes. |
| `deprecation-help` | [search][search-atom-repo-label-deprecation-help] | [search][search-atom-org-label-deprecation-help] | Issues for helping package authors remove usage of deprecated APIs in packages. |
| `electron` | [search][search-atom-repo-label-electron] | [search][search-atom-org-label-electron] | Issues that require changes to [Electron](https://electron.atom.io) to fix or implement. |

#### Pull Request Labels

| Label name | `atom/atom` :mag_right: | `atom`‑org :mag_right: | Description
| --- | --- | --- | --- |
| `work-in-progress` | [search][search-atom-repo-label-work-in-progress] | [search][search-atom-org-label-work-in-progress] | Pull requests which are still being worked on, more changes will follow. |
| `needs-review` | [search][search-atom-repo-label-needs-review] | [search][search-atom-org-label-needs-review] | Pull requests which need code review, and approval from maintainers or Atom core team. |
| `under-review` | [search][search-atom-repo-label-under-review] | [search][search-atom-org-label-under-review] | Pull requests being reviewed by maintainers or Atom core team. |
| `requires-changes` | [search][search-atom-repo-label-requires-changes] | [search][search-atom-org-label-requires-changes] | Pull requests which need to be updated based on review comments and then reviewed again. |
| `needs-testing` | [search][search-atom-repo-label-needs-testing] | [search][search-atom-org-label-needs-testing] | Pull requests which need manual testing. |
