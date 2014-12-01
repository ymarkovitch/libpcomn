#########################################################################
# ^FILE: cmdparse.pl - cmdparse for perl programs
#
# ^DESCRIPTION:
#    This file defines a perl function named cmdparse to parse
#    command-line arguments for perl scripts.
#
# ^HISTORY:
#    05/14/91	Brad Appleton	<bradapp@enteract.com>	Created
##^^#####################################################################


########
# ^FUNCTION: cmdparse - parse command-line argument vectors
#
# ^SYNOPSIS:
#    eval  &cmdparse(@args);
#
# ^PARAMETERS:
#    args -- The vector of arguments to pass to cmdparse(1);
#            This will usually be the list ("-decls=$ARGS", "--", $0, @ARGV)
#            where $ARGS is the variable containing all the argument
#            declaration strings.
#
# ^DESCRIPTION:
#    Cmdparse will invoke cmdparse(1) to parse the command-line.
#
# ^REQUIREMENTS:
#    Any desired initial values for variables from the argument-description
#    string should be assigned BEFORE calling this function.
#
# ^SIDE-EFFECTS:
#    Terminates perl-script execution if command-line syntax errors are found
#
# ^RETURN-VALUE:
#    A string of perl-variable settings to be evaluated.
#
# ^EXAMPLE:
#     #!/usr/bin/perl
#
#     require  'cmdparse.pl';
#
#     $ARGS = '
#       ArgStr   string  "[S|Str string]" : STICKY    "optional string argument"
#       ArgStr   groups  "[g|groups newsgroups ...]"  "groups to test"
#       ArgInt   count   "[c|count number]"           "group repeat count"
#       ArgStr   dirname "[d|directory pathname]"     "directory to use"
#       ArgBool  xflag   "[x|xmode]"                  "turn on X-mode"
#       ArgClear yflag   "[y|ymode]"                  "turn off Y-mode"
#       ArgChar  sepch   "[s|separator char]"         "field separator"
#       ArgStr   files   "[f|files filenames ...]"    "files to process"
#       ArgStr   name    "[n|name] name"              "name to use"
#       ArgStr   ARGV    "[args ...]"                 "any remaining arguments"
#     ';
#
#     $count = 1;
#     $dirname = '.';
#     $sepch = ',';
#     $yflag = 'TRUE';
#
#     eval &cmdparse("-decls=$ARGS", "--", $0, @ARGV);
#
##^^####

sub cmdparse {
   local(@args) = @_ ;
   local($output) = ("");
   local($nforks, $tmpfile, $tmpdir, $exitrc, $_) = (0, "tmp$$");

   $tmpdir = $ENV{'TMP'};  ## use ${TMP:-/tmp}/tmp$$ as the temporary file
   if (! $tmpdir) {
      $tmpdir = '/tmp';
   }
   $tmpfile = $tmpdir . '/' . $tmpfile;

   ## I could just call cmdparse(1) using `cmdparse <options> <args>`
   ## but then I would need to escape all shell meta-characters in each
   ## argument. By using exec(), the arguments are passed directly to
   ## the system and are not "globbed" or expanded by the shell.
   ##
   ## Hence I will need to fork off a child, redirect its standard output
   ## to a temporary file, and then exec cmdparse(1).

FORK: {
      ++$nforks;
      if ($pid = fork) {
            # parent here
         waitpid($pid, 0);  ## wait for child to die
         $exitrc = $?;
         $output = `cat $tmpfile` unless $exitrc;   ## save the output-script
         unlink($tmpfile);  ## remove the temporary file
         if ($exitrc) {
            $! = 0;
            die "\n";
         }
      } elsif (defined $pid) { ## pid is zero here if defined
            # child here
         open(STDOUT, "> $tmpfile") || die "Can't redirect stdout";
         exec("cmdparse", "-shell=perl", @args);
      } elsif ($! =~ /No more process/ ) {
            # EAGAIN, supposedly recoverable fork error
         if ($nforks > 10) {
            die "$0: Can't fork cmdparse(1) after 10 tries.\n" ;
         } else {
            sleep 1;
            redo FORK;
         }
      } else {
         die "$0: Can't fork cmdparse(1): $!\n" ;
      }
   } ##FORK

   return $output;
}

1;
