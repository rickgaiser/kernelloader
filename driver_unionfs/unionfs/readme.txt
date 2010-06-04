Copyright (c) 2003-2005 Erez Zadok <ezk@cs.sunysb.edu>
Copyright (c) 2003-2005 Charles P. Wright
Copyright (c) 2003-2005 Stony Brook University
Copyright (c) 2003-2005 The Research Foundation of State University of New York

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

We're pleased to announce the first release of a unioning file system
for Linux, called Unionfs.  To download software and documentation,
see

	http://www.fsl.cs.sunysb.edu/project-unionfs.html

Unionfs is a stackable unification file system, which can appear to
merge the contents of several directories (branches), while keeping
their physical content separate.  Unionfs is useful for unified source
tree management, merged contents of split CD-ROM, merged separate
software package directories, data grids, and more.  Unionfs allows
any mix of read-only and read-write branches, as well as insertion and
deletion of branches anywhere in the fan-out.  To maintain Unix
semantics, Unionfs handles elimination of duplicates, partial-error
conditions, and more.  This release also includes additional
preliminary features that were specifically designed for security
applications, such as snapshotting and sandboxing.

This Unionfs release has been tested with recent 2.4 and 2.6 kernels.
For the 2.4 kernels, the minimum requirement is 2.4.20.  For 2.6
kernels, the minimum requirement is 2.6.11.  You also need to have the
development headers for e2fsprogs installed.

Unionfs is released under the GPL (see the COPYING file in the
distribution for details).

For more information on using Unionfs, download the tarball and see
the following man pages:

- unionfs.4:  Describes how to mount unionfs

- unionctl.8: Describes how to control an already mounted Union

For more information about Unionfs internals (which we think are
really cool :-), see the following technical report at the above Web
site:

  C. P. Wright, J. Dave, P. Gupta, H. Krishnan, E. Zadok, and M. Zubair
  "Versatility and Unix Semantics in a Fan-Out Unification File System"
  Technical Report FSL-04-01b, October 2004
  Computer Science Department, Stony Brook University
  http://www.fsl.cs.sunysb.edu/docs/unionfs-tr/unionfs.pdf

In addition, you can find an article in Linux Journal (December 2004
issue) titled "Unionfs: Bringing File Systems Together."  It is available
online at http://www.linuxjournal.com/article/7714.

See the INSTALL file for instructions on building Unionfs, iteractions with
kernel features, and known bugs and limitations.

To report bugs, please email them to the "fistgen@filesystems.org"
list (see www.filesystems.org), or submit them via Bugzilla to
https://bugzilla.filesystems.org/.  But reports with fixes are most
welcome.

Enjoy,
Erez and Charles (on behalf of the Unionfs team)

##############################################################################
## For Emacs:
# Local variables:
# fill-column: 70
# End:
##############################################################################
