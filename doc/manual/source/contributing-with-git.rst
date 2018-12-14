.. include:: replace.txt
.. highlight:: cpp

.. Section Separators:
   ----
   ****
   ++++
   ====
   ~~~~

.. _Contributing with git:

Contributing with git
---------------------

The ns-3 project used Mercurial in the past as its source code control system, but is has moved to Git in December 2018. Git is a VCS like Mercurial, Subversion or CVS, and it used to maintain many open-source (and closed-source) project. While git and mercurial have a lot of common properties, if you are new to git you should read first an introduction to it. The most up-to-date guide is the [Git Book](https://git-scm.com/book/en/v2/Getting-Started-Git-Basics Git Book).

Ns-3 project is officially hosted on GitLab.com at [this address](https://gitlab.com/nsnam/ns-3-dev).

This section of the manual provides common tips for both users and maintainers. Since the first part is common, we will start a personal repository setup and a first example of contribution.

Setup of a personal repository
******************************

We will provide two ways, one anonymous (but will impede the creation of merge requests) and the other, preferred, that include forking the repository through the GitLab.com web interface.

Anonumously cloning ns-3-dev
++++++++++++++++++++++++++++

If you go to the [official ns-3-dev page](https://gitlab.com/nsnam/ns-3-dev) you can find a button that says `Clone`. If you are not logged in, then you will find only the option of cloning the repository through HTTPS, with this command:

.. sourcecode:: bash

   $ git clone https://gitlab.com/nsnam/ns-3-dev.git

If this command exits successfully, you will have a newly created `ns-3-dev` directory with all the source code.

Forking ns-3-dev on GitLab.com
++++++++++++++++++++++++++++++

Assume that you are user 'john' on GitLab.com, and that you want to create a new repository that is synced with nsnam/ns-3-dev.

#. Log into GitLab.com
#. Navigate to https://gitlab.com/nsnam/ns-3-dev
#. In the top-right corner of the page, click '''Fork'''.

Note that you may only do this once; if you try to Fork again, Gitlab will take you to the page of the original fork.

For more information on forking with Gilab, there is plenty of visual [documentation](https://docs.gitlab.com/ee/gitlab-basics/fork-project.html).

Clone your repository on your machine
=====================================

Clone the newly created fork to your system going to the homepage of your fork (that should be in the form `https://gitlab.com/your-user-name/ns-3-dev`) and click the `Clone` button. Then, go to your computer's terminal, and issue the [command](https://docs.gitlab.com/ee/gitlab-basics/command-line-commands.html#clone-your-project):

.. sourcecode:: bash

   $ git clone https://gitlab.com/your-user-name/ns-3-dev
   $ cd ns-3-dev-git

Add the remote to an existing ns-3-dev installation
===================================================

If you already used git in the past, you probably have


== Typical workflow ==

Git is a distributed versioning system. This means that '''nobody''' will touch your personal repo, until you do something. Please note that every github user has, at least, two repositories: the first is represented by the repository hosted on github servers, which will be called in the following ''origin''. Then, you have your clone on your machine. This means that you could have many clones, on different machines, which refers to ''origin''.

=== Add the official ns-3 repository as remote upstream ===

When you fork a repository, your history is no more bound to the repository itself. At this point, it is your duty to sync your fork with the original repository.
Git is able to fetch and push changes to several repositories, each of them is called '''remote'''. The first remote repository we have encountered is ''origin''; we must add the official ns-3 repo as another remote repository.

  git remote add upstream https://github.com/nsnam/ns-3-dev-git

With the command above, we added a remote repository, named upstream, which links to the official ns-3 repo. To show your remote repositories:

  git remote show

To see to what ''origin'' is linking to:

  git remote show origin

You can also delete remotes:

  git remote remove [name]

Many options are available; please refer to the git manual for more.

=== Sync your repo ===

We assume, from now to the end of this document, that you will not make commits on top of the master branch. It should be kept clean from '''any''' personal modifications. Move the current HEAD to the master branch:

  git checkout master

and fetch the changes from ''upstream'':

  git fetch upstream

we can now merge the official changes into our master:

  git pull upstream master

If you tried a pull which resulted in complex conflicts and would want to start over, you can recover with git reset (but this never happens if you do not commit over master).

=== Start a new branch ===

Now look at the available branches:

  git branch -a

you should see:

  * master
    remotes/origin/master
    remotes/upstream/master

The branch master is your local master branch; remotes/origin/master point at the master branch on your repository located in the Github server, while remotes/upstream/master points to the official master branch.

To create a new branch, which hopefully will contains new feature or bug correction, the command is

  git checkout -b [name_of_your_new_branch]

To switch between branches, remove the -b option. You should now see:

  git branch -a

  * master
    [name_of_your_new_branch]
    remotes/origin/master
    remotes/upstream/master

=== Edit and commit the modifications ===

After you edit some file, you should commit the difference. As a policy, git users love small and incremental patches. So, commit early, and commit often: you could rewrite your history later.

Suppose we edited src/internet/model/tcp-socket-base.cc. With git status, we can see the repostory status:

 <nowiki>
  On branch tcp-next
  Your branch is up-to-date with 'mirror/tcp-next'.
  Changes not staged for commit:
             modified:   src/internet/model/tcp-socket-base.cc
</nowiki>
and we can see the edits with git diff:

 <nowiki>
nat@miyamoto ~/Work/ns-3-dev-git (tcp-next)$ git diff
diff --git i/src/internet/model/tcp-socket-base.cc w/src/internet/model/tcp-socket-base.cc
index 1bf0f69..e2298b0 100644
--- i/src/internet/model/tcp-socket-base.cc
+++ w/src/internet/model/tcp-socket-base.cc
@@ -1439,6 +1439,10 @@ TcpSocketBase::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
       // There is a DupAck
       ++m_dupAckCount;

+      // I'm introducing a subtle bug!
+
+      m_tcb->m_cWnd = m_tcb->m_ssThresh;
+
       if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
         {
           // From Open we go Disorder
</nowiki>

To create a commit, select the file you want to add to the commit with git add:

  git add src/internet/model/tcp-socket-base.cc

and then commit the result:

  git commit -m "My new TCP broken"

You can see the history of the commits with git log. To show a particular commit, copy the sha id and use git show <sha-id> .
Do this until you feature or bug is corrected; now, it is time to make a review request.

=== Rebase your branch on master ===

Meanwhile you were busy with your branch, the upstream master could have changed. To rebase your work with the now new master, first of all sync your master branch as explained before; then

  git checkout [name_of_your_new_branch]
  git rebase master

this will rewind your work, update the HEAD of your branch to the actual master, and then re-apply all your work. If some of your work conflicts with the actual master, you will be asked to fix these conflicts if automatic merge fails.

=== Creating a patch against master ===

If you are in a branch and want to diff it against master, you can type:

  git diff master

and redirect to a patch:

  git diff master > patch-against-master.patch

To preserve all your commits, it is preferable to create many patches, one for each commit:

  git format-patch master

and then you could submit them for a review.

=== Rewrite your history ===

Let's suppose that, after a first round of review, you are asked to fix something. After you commited the fix of your (previously bad) commit, the log should be something like:

 <nowiki>
nat@miyamoto ~/Work/ns-3-dev-git (tcp-next)$ git log
Sun Jan 17 14:43:36 2016 +0100 9cfa78e (HEAD -> tcp-next) Fix for the previous  [Tony Spinner]
Sun Jan 17 14:43:16 2016 +0100 77ad6e1 first version of TCP broken  [Tony Spinner]
Mon Jan 11 14:44:13 2016 -0800 e2fc90a (origin/master, upstream/master, master) update some stale tutorial info [Someone]
Mon Jan 11 21:11:54 2016 +0100 1df9c0d Bug 2257 - Ipv[4,6]InterfaceContainer::Add are not consistent [Someone]
</nowiki>

If you plan to resubmit a review, you would like to merge the commits (original and its fix). You can easily do this with git rebase -i (--interactive).

  git rebase -i master

and an editor should pop up, with your commits, and various possibilities:

 <nowiki>
pick 77ad6e1 first version of TCP broken
pick 9cfa78e Fix for the previous

# Rebase e2fc90a..9cfa78e onto e2fc90a (10 command(s))
#
# Commands:
# p, pick = use commit
# r, reword = use commit, but edit the commit message
# e, edit = use commit, but stop for amending
# s, squash = use commit, but meld into previous commit
# f, fixup = like "squash", but discard this commit's log message
# x, exec = run command (the rest of the line) using shell
# d, drop = remove commit
#
# These lines can be re-ordered; they are executed from top to bottom.
#
# If you remove a line here THAT COMMIT WILL BE LOST.
#
# However, if you remove everything, the rebase will be aborted.
#
# Note that empty commits are commented out
</nowiki>

What we want is to "fixup" the fix commit into the previous. So, changing "pick" to "fixup" (or simply f) and saving and quitting the editor is sufficient. The edited content:

 <nowiki>
pick 77ad6e1 first version of TCP broken
f 9cfa78e Fix for the previous
</nowiki>

The resulting history:

 <nowiki>
nat@miyamoto ~/Work/ns-3-dev-git (tcp-next)$ git log
Sun Jan 17 14:43:16 2016 +0100 847e9a8 (HEAD -> tcp-next) first version of TCP broken  [Tony Spinner]
Mon Jan 11 14:44:13 2016 -0800 e2fc90a (origin/master, upstream/master, master) update some stale tutorial info [Someone]
Mon Jan 11 21:11:54 2016 +0100 1df9c0d Bug 2257 - Ipv[4,6]InterfaceContainer::Add are not consistent [Someone]
</nowiki>

Please remember that rewriting already published history is '''bad''', especially if someone other has synced your commits into their repository, but is necessary if you want to make clear commits. As a general rule, your personal development history can be rewritten as you wish; the history of your published features should be kept as they are.

== Collaborating: an example of exchange between a maintainer and a contributor ==
In the following we will explain a typical git workflow in the case of an exchange between a maintainer (M) and a contributor (C). It's clear that M is the only one that can push over the official ns-3-dev repo; but sometime, commits from C should be reviewed and tested out-of-the-tree. Now, we will follow two paths, corresponding to the actions of C and the actions of M.

=== How a contributor could made commits to be reviewed by a maintainer ===

The first step for C is to set up a fork of the official ns-3-dev-git repository (see before). Then, if the M has it own repository, with some branches dedicated to future work that can be included next in the ns-3-dev repository, C should add its remote and fetch the index:

  git remote add [name] [url]
  git fetch [name]

For instance, let's assume we are working on TCP. Nat usually has his own "next-to-be-pushed" work in the branch "tcp-next" of his repo on github. Therefore, C should start its work from this branch.

  git remote add nat https://github.com/kronat/ns-3-dev-git
  git fetch
  git checkout -b [feature_name] nat/tcp-next

Please note that this is an example; the repo name, address, and the branch name could be different. At this point, the (local) branch of C called [feature_name] is tracking nat/tcp-next, and so it has all the same commits. C can start to work, using the normal workflow.

When the set of commit is ready, C can push the branch to its own repository. Just do:

  git push

NO ! This will likely results in an error, since C does not have the permission into writing the M repo. C should create a new branch in his own repository:

  git push -u origin [feature_name]

That command will change the upstream reference, setting it to your origin (which is likely your github repo), and pushing all the work into the remote "origin". For a visual representation of what is happening:

<nowiki>
[commit 169] <----- C/tcp-next
....                       ( work of C on tcp-next )
[commit 160] <----- nat/tcp-next
....                      (previous work on TCP)
[commit 123]  <---- ns-3-dev-git
...                      (official ns-3 history)
</nowiki>

Well, at this point C could notify M: "Please, see my work at github.com/C/ns-3-dev-git in the branch [feature_name]!" and maybe asking for a merge. And it's time to change the perspective, passing to M.

=== A maintainer with a lot of contributor ===

M has been notified of the work of C. The first thing he has to do is to adding the C repository to his (long) list of remotes:

  git remote add [C_name] [C_repo]
  git fetch [C_name]

Then, he could quickly view the work done by the contributor:

  git checkout -b [local_C_name] [C_name]/[C_branch]

If M thinks the work is well-done and ok, he could directly merge it into his branch:

  git checkout tcp-next
  git checkout merge [local_C_name]

At this point, after some time, if the works is ok M should push the changes to the ns-3-dev mercurial repo. This is really tedious, and should be done carefully.. And it will be explained later in this HOWTO. But, as it often happens, M could made comments, and then wait another patchset.

  git checkout master
  git branch -D [local_C_name]

=== A quick flash-back to C ===

Often the maintainer will made changes to that branch, because works of other contributor could have been merged, or whatever. So, C should be ready to _REBASE_ his own branch. Please note that a maintainer is *forced* to merge the contributor's work, because otherwise will broke the history of many. But each single contributor should *rebase* his work on the maintainer branch (or the master if the maintainer has no one). So, please go back and re-read carefully the section for rebasing the work.

=== Pushing things to ns-3-dev mercurial repository ===

Of course this applies only for maintainers. When you have a local branch (e.g. tcp-next) that is ready to be merged in ns-3-dev, please do the following:
# Pull the ns-3-dev-git master into your master branch
# Rebase your local tcp-next branch to your local master branch
# Create a series of patch that applies on top of master:
  git format-patch master
# Copy these patches in ns-3-dev mercurial repo
# Apply and commit one-by-one all these patches, paying attention to the right owner of the commit.

One last thing: for binary patches this doesn't work: you need to update by hand all the files. We still need to figure a way to export correctly binary patches from git to mercurial.

== Cheat sheet ==
=== Removing all local changes not tracked in the index (Dangerous) ===

  git clean -df
  git checkout -- .
