

# Commit Permissions #

The repository is read-only to everybody, for commit access please send an email to `uqs`.

You need to use a special token as password, not your Google or Gmail password. Go to your Profile page, then click Settings where you can (re-)generate a googlecode.com password token.

# How to Check Out #
## Subversion ##

```
$ svn checkout https://avalonsailing.googlecode.com/svn/trunk avalonsailing --username <you>@google.com
```
Upon your first commit, it will ask for the password for `<you>(@gmail.com)`, this will fail. Then it asks for your username (enter `<you>@google.com`) and your password-token

**from** **https://code.google.com/hosting/settings**

. Voilà.

## Mercurial ##

For the mercurial--subversion bridge to work, you have to install the Python bindings for subversion and a special mercurial module.

```
$ sudo apt-get install python-subversion
$ hg clone http://bitbucket.org/durin42/hgsubversion/
```

Then put the following in your `~/.hgrc`:
```
[extensions]
rebase =
transplant =
hgsubversion = ~/hgsubversion/hgsubversion
```

Now you're ready to clone the svn repo:
```
$ hg clone svn+https://avalonsailing.googlecode.com/svn avalonsailing
```

Use `hg pull` to fetch the latest updates, try not to commit to the default branch as mercurial will not allow you to easily re-write this history for pushing into svn (you should only push the final/complete commit, not all the fixups after code review).

See http://mercurial.selenic.com/wiki/MqTutorial for mutable changesets.

Easier to use is the histedit extension at https://bitbucket.org/durin42/histedit/src see [HisteditExtension](http://mercurial.selenic.com/wiki/HisteditExtension) on how to use it.

```
$ hg pull #do this while on the default branch
$ hg branch myfeature
$ hg add;hg commit; etc. pp.
# upload patch to Rietveld, once review is finished:
$ hg histedit -o  # or use -r and give exact revisions, fold all commits into one
$ hg up -C default && hg pull
$ hg rebase -s myfeature -d default
$ hg out # double check everything
$ hg push
```

After rebasing your work onto the default branch, use `hg out` to check what would be send upstream. Then run `hg push` for the actual svn commit.

## Git ##

Get the git-svn bridge module, then clone our svn repo:
```
$ sudo apt-get install git-svn
$ git svn clone --stdlayout https://avalonsailing.googlecode.com/svn avalonsailing
```

This will fetch into the remote branch `trunk` and you'll end up with a local `master` branch, where you should commit your first patches. Before submitting them to svn do `git rebase -i trunk master` to get your local commits into shape for the offical repo. Then run `git svn dcommit -n` to see what would be done, drop the `-n` for the real commit.

# Review Process #

Register an account at http://codereview.appspot.com/ and download [upload.py](http://codereview.appspot.com/static/upload.py), put it in your `$PATH` and make it exectuable.

## Subversion ##

Create your patch as you would usually do, `svn add`ing files and checking the output of `svn diff`, but don't commit yet. Use
```
upload.py --send_mail -r <list-of-reviewers> --cc=avalonsailing@googlegroups.com
```
to send this patchset to Rietveld and the listed reviewers. After you have included their comments, use upload again to send further patches, but include `-i <issue-number>` so it does not generate a new issue.

Finally `svn ci` with a commit message template as given below.

## Mercurial ##

CAVEAT: mostly untested

Clone the repository or create a named branch with your commits, use
```
upload.py --send_mail -r <list-of-reviewers> --cc=avalonsailing@googlegroups.com --rev=default:branch
```
or the direct revision numbers of your patch that you'd like to upload to Rietveld. After you have included the review comments, use upload again to send further patches, but include `-i <issue-number>` so it does not generate a new issue.

Finally `hg push` with a commit message template as given below.

## Git ##

Clone the repository or create a named branch with your commits, use
```
upload.py --send_mail -r <list-of-reviewers> --cc=avalonsailing@googlegroups.com --rev=trunk:master
```
or the direct revision numbers of your patch that you'd like to upload to Rietveld. After you have included the review comments, use upload again to send further patches, but include `-i <issue-number>` so it does not generate a new issue.

Finally `git svn dcommit` with a commit message template as given below.

# Commit Message Template #

Try to use something like the following as a commit message format. Sadly, we cannot enforce this on the subversion side of code.google.com.


---

```
component: Short 1-line description of what changed

Long explanation of *why* things were changed, etc.

R=<list of reviewers>
I=<code review issue numbers>
B=<bug numbers> (this line is optional)
```

---
