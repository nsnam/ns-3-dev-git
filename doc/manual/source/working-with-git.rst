.. include:: replace.txt
.. highlight:: cpp

.. Section Separators:
   ----
   ****
   ++++
   ====
   ~~~~

.. _Working with git:

Working with git as a user
--------------------------

The ns-3 project used Mercurial in the past as its source code control system, but it has moved to Git in December 2018. Git is a VCS like Mercurial, Subversion or CVS, and it is used to maintain many open-source (and closed-source) projects. While git and mercurial have a lot of common properties, if you are new to git you should read first an introduction to it. The most up-to-date guide is the Git Book, at https://git-scm.com/book/en/v2/Getting-Started-Git-Basics.

The ns-3 project is officially hosted on GitLab.com at https://gitlab.com/nsnam/.  For convenience and historical reasons, ns-3-dev mirrors are currently posted on Bitbucket.com and GitHub.com, and kept in sync with the official repository periodically via cron jobs.  We recommend that users who have been working from one of these mirrors repoint their remotes so that they pull origin or upstream from GitLab.com (see below explanation about how to configure remotes).

This section of the manual provides common tips for both users and maintainers. Since the first part is shared, in this manual section we will start a personal repository and then explain what to do in some typical cases. ns-3 users often combine ns-3-dev with other repositories (pybindgen, netanim, apps from the app store).  This manual chapter does not cover this use case; it only focuses on the single ns-3-dev repository.  See other project documentation such as the ns-3 tutorial for descriptions on bundled releases distributed as source archives, or on the bake build tool for managing multiple repositories.  The guidelines listed below also largely pertain to the user who is using (and cloning) bake from the GitLab.com repository.

ns-3's Git workflow in a nutshell
*********************************

Experienced git users will not necessarily need instruction on how to set up personal repositories (below).  However, they should be aware of the project's workflow:

* The main repository's master branch is the main development branch.
* Releases are tagged changesets on the master branch.  If a hotfix release must be made from a changeset in the past, a new hotfix support branch will be created and maintained for any such releases.  Hotfix releases will be tagged from the hotfix support branch if it has diverged from master.
* Maintainers can commit obvious non-critical fixes (documentation improvements, typos etc.) directly into the master branch.  Users who are not maintainers can create Merge Requests for small items such as these.
* For each new major feature, create a feature branch in your own fork.  Create Merge Requests as needed to solicit review.  Merge feature branches into master after review.  Merged feature branches can then be deleted.

Setup of a personal repository
******************************

We will provide two ways, one anonymous (but will impede the creation of merge requests) and the other, preferred, that include forking the repository through the GitLab.com web interface.

Directly cloning ns-3-dev
+++++++++++++++++++++++++

If you go to the official ns-3-dev page, hosted at https://gitlab.com/nsnam/ns-3-dev, you can find a button that says ```Clone``. If you are not logged in, then you will see only the option of cloning the repository through HTTPS, with this command:

.. sourcecode:: bash

   $ git clone https://gitlab.com/nsnam/ns-3-dev.git

If this command exits successfully, you will have a newly created `ns-3-dev` directory with all the source code.

Forking ns-3-dev on GitLab.com
++++++++++++++++++++++++++++++

Assume that you are the user *john* on GitLab.com and that you want to create a new repository that is synced with nsnam/ns-3-dev.

#. Log into GitLab.com
#. Navigate to https://gitlab.com/nsnam/ns-3-dev
#. In the top-right corner of the page, click ``Fork``.

Note that you may only do this once; if you try to fork again, Gitlab will take you to the page of the original fork. So, if you are planning to maintain two or more separate forks (for example, one for your private work, another for maintenance, etc.), you are doing a mistake. Instead, you should add these forks as a remote of your existing directory (see below for adding remotes). Usually, it is a good thing to add the maintainer's repository as remotes, because it can happen that "bleeding edge" features will appear there before landing in ns-3.

For more information on forking with Gilab, there is plenty of visual documentation (https://docs.gitlab.com/ee/gitlab-basics/fork-project.html). To work with your forked repository, you have two way: one is a clean clone while the other is meant to re-use an existing ns-3 git repository.

Clone your forked repository on your machine
============================================

Git is a distributed versioning system. This means that *nobody* will touch your personal repository, until you do something. Please note that every gitlab user has, at least, two repositories: the first is represented by the repository hosted on gitlab servers, which will be called in the following ``origin``. Then, you have your clone on your machine. This means that you could have many clones, on different machines, which points to ``origin``.

To clone the newly created fork to your system go to the homepage of your fork (that should be in the form `https://gitlab.com/your-user-name/ns-3-dev`) and click the `Clone` button. Then, go to your computer's terminal, and issue the command (please refer to https://docs.gitlab.com/ee/gitlab-basics/command-line-commands.html#clone-your-project for more documentation):

.. sourcecode:: bash

   $ git clone https://gitlab.com/your-user-name/ns-3-dev
   $ cd ns-3-dev-git

In this example we used the HTTPS address because in some place the git + ssh address is blocked by firewalls. If you are not under this constraint, it is recommended to use the git + ssh address to avoid the username/password typing at each request.

Naming conventions
==================

Git is able to fetch and push changes to several repositories, each of them is called ``remote``. With time, you probably will have many remotes, each one with many branches. To avoid confusion, it is recommended to give meaningful names to the remotes; in the following, we will use ``origin`` to indicate the ns-3-dev repository in your personal namespace (your forked version, server-side) and ``nsnam`` to indicate the ns-3-dev repository in the nsnam namespace, server-side.

Add the official ns-3 repository as remote upstream
***************************************************

You could have already used git in the past, and therefore already having a ns-3 git repository somewhere. Or, instead, you could have it cloned for the first time in the step above. In both cases, when you fork/clone a repository, your history is no more bound to the repository itself. At this point, it is your duty to sync your fork with the original repository. The first remote repository we have encountered is ``origin``; we must add the official ns-3 repo as another remote repository:

.. sourcecode:: bash

   $ git remote add nsnam https://gitlab.com/nsnam/ns-3-dev

With the command above, we added a remote repository, named upstream, which links to the official ns-3 repo. To show your remote repositories:

.. sourcecode:: bash

   $ git remote show

To see to what ``origin`` is linking to:

.. sourcecode:: bash

   $ git remote show origin

Many options are available; please refer to the git manual for more.

Add your forked repository as remote
************************************

If you were a user of the old github mirror, you probably have an existing git repository installed somewhere. In your case, it is not necessary to clone your fork and to port all your work in the new directory; you can add the fork as new remote.

.. sourcecode:: bash

   $ git remote rename origin old-origin
   $ git remote add origin https://gitlab.com/your-user-name/ns-3-dev

After these two commands, you will have a remote, named origin, that points
to your forked repository on gitlab.

Keep in sync your repository with latest ns-3-dev updates
*********************************************************

We assume, from now to the end of this document, that you will not make commits on top of the master branch. It should be kept clean from *any* personal modifications: all the works must be done in branches. Therefore, to move the current HEAD of the master branch to the latest commit in ns-3-dev, you should do:

.. sourcecode:: bash

   $ git checkout master
   $ git fetch nsnam
   $ git pull nsnam master

If you tried a pull which resulted in a conflict and you would like to start over, you can recover with git reset (but this never happens if you do not commit over master).

Start a new branch to do some work
**********************************

Look at the available branches:

.. sourcecode:: bash

   $ git branch -a

you should see something like:

.. sourcecode:: bash

   * master
     remotes/origin/master
     remotes/nsnam/master

The branch master is your local master branch; remotes/origin/master point at the master branch on your repository located in the Gitlab server, while remotes/nsnam/master points to the official master branch.

Before entering in details on how to create a new branch, we have to explain why it is recommended to do it. First of all, if you put all your work in a separate branch, you can easily see the diff between ns-3 mainline and your feature branch (with ``git diff master``). Also, you can integrate more easily the upstream advancements in your work, and when you wish, you can create a *conflict-free* merge request, that will ease the maintainer's job in reviewing your work.

To create a new branch, starting from master, the command is:

.. sourcecode:: bash

   $ git checkout master
   $ git checkout -b [name_of_your_new_branch]

To switch between branches, remove the -b option. You should now see:

.. sourcecode:: bash

   $ git branch -a
    * master
     [name_of_your_new_branch]
     remotes/origin/master
     remotes/upstream/master

Edit and commit the modifications
*********************************

After you edit some file, you should commit the difference. As a policy, git users love small and incremental patches. So, commit early, and commit often: you could rewrite your history later.

Suppose we edited ``src/internet/model/tcp-socket-base.cc``. With git status, we can see the repository status:

.. sourcecode:: bash

   $ git status
      On branch tcp-next
      Your branch is up-to-date with 'mirror/tcp-next'.
      Changes not staged for commit:
        modified:   src/internet/model/tcp-socket-base.cc

and we can see the edits with git diff:

.. sourcecode:: text

   $ git diff

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


To create a commit, select the file you want to add to the commit with git add:

.. sourcecode:: bash

   $ git add src/internet/model/tcp-socket-base.cc

and then commit the result:

.. sourcecode:: bash

   $ git commit -m "My new TCP broken"

Of course, it would be better to have some rules for the commit message: they will be reported in the next subsection.

Commit message guidelines
+++++++++++++++++++++++++

The commit title should not go over the 80 char limit. It should be prefixed by the name of the module you are working on, and if it fixes a bug, it should reference it in the commit title. For instance, a good commit title would be:

 tcp: My new TCP broken

Another example is:

 tcp: (fixes #2322) Corrected the uint32_t wraparound during recovery

In the body message, try to explain what the problem was, and how you resolved that. If it is a new feature, try to describe it at a very high level, and highlight any modifications that changed the behaviour or the interface towards the users or other modules.

Commit log
++++++++++

You can see the history of the commits with git log. To show a particular commit, copy the sha-id and use ``git show <sha-id>``. The ID is unique, so it can be referenced in emails or in issues. The next step is useful if you plan to contribute back your changes, but also to keep your feature branch updated with the latest changes from ns-3-dev.

Rebase your branch on top of master
***********************************

Meanwhile you were busy with your branch, the upstream master could have changed. To rebase your work with the now new master, first of all sync your master branch (pulling the nsnam/master branch into your local master branch) as explained before; then

.. sourcecode:: bash

    $ git checkout [name_of_your_new_branch]
    $ git rebase master

The last command will rewind your work, update the HEAD of your branch to the actual master, and then re-apply all your work. If some of your work conflicts with the actual master, you will be asked to fix these conflicts if automatic merge fails.

Pushing your changes to origin
******************************

After you have done some work on a branch, if you would like to share it with others there is nothing better than pushing your work to your origin repository, on Gitlab servers.

.. sourcecode:: bash

    $ git checkout [name_of_your_new_branch]
    $ git push origin [name_of_your_new_branch]

The ``git push`` command can be used every time you need to push something from your computer to a remote repository, except when you propose changes to the main ns-3-dev repository: your changes must pass a review stage.

Please note that for older git version, the push command looks like:

.. sourcecode:: bash

    $ git push -u origin [name_of_your_new_branch]


Submit work for review
**********************

After you push your branch to origin, you can follow the instructions here https://docs.gitlab.com/ee/gitlab-basics/add-merge-request.html to create a merge request. Please remember to add, as reviewer, at least one maintainer. To get the information on who is maintaining what, please refer to the nsnam website.

Porting patches from mercurial repositories to git
**************************************************

  Placeholder section. Please improve it.

Working with git as a maintainer
--------------------------------

As a maintainer, you are a person who has write access to the main nsnam repository. You could push your own work (without passing from code review) or push someone else's work. Let's investigate the two cases.

Pushing your own work
*********************

Since you have been added to the Developer list on Gitlab (if not, please open an issue) you can use the git + ssh address when adding nsnam as remote. Once you have done that, you can do your modifications to a local branch, then update the master to point to the latest changes of the nsnam repo, and then:

.. sourcecode:: bash

    $ git checkout master
    $ git pull nsnam master
    $ git merge [your_branch_name]
    $ git push nsnam master

Please note that if you want to keep track of your branch, you can use as command ``git merge --no-ff [your_branch_name]``. It is always recommended to rebase your branch before merging, to have a clean history. That is not a requirement, though: git perfectly handles a master with parallel merged branches.

Review and merge someone else's work
************************************

Gitlab.com has a plenty of documentation on how to handle merge requests. Please take a look here: https://docs.gitlab.com/ee/user/project/merge_requests/index.html.

If you are committing a patch from someone else, and it is not coming through a Merge Request process, you can use the --author='' argument to 'git commit' to assign authorship to another email address (such as we have done in the past with the Mercurial -u option).
