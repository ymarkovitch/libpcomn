#########################################################################
# ^FILE: cmdparse.tcl - cmdparse for tcl scripts
#
# ^DESCRIPTION:
#    This file defines a tcl procedure named cmdparse to parse
#    command-line arguments for tcl scripts.
#
# ^HISTORY:
#    05/07/92  Brad Appleton   <bradapp@enteract.com>   Created
##^^#####################################################################

########
# ^PROCEDURE: cmdparse - parse command-line argument lists
#
# ^SYNOPSIS:
#    cmdparse <options> -- $scriptName $list
#
#        where <options> is any valid option combination for cmdparse(1),
#        $scriptName is the name of the user's tcl script (or procedure),
#        and $list is a list (which will usually be $argv for scripts, and
#        and $args for procedures).
#
# ^DESCRIPTION:
#    Parseargs will invoke cmdparse(1) with the options and arguments
#    specified by the caller.
#
# ^REQUIREMENTS:
#    Any desired initial values for variables from the argument-description
#    string should be assigned BEFORE calling this procedure.
#
# ^SIDE-EFFECTS:
#    If cmdparse(1) exits with a non-zero status, then execution
#    is terminated.
#
# ^RETURN-VALUE:
#    A string of variable settings for the caller to evaluate.
#
# ^EXAMPLE:
#     #!/usr/local/bin/tcl
#
#     load  "cmdparse.tcl"
#
#     set arguments {
#        ArgStr   string  "[S|Str [string]]"          "optional string arg"
#        ArgStr   groups  "[g|groups newsgroups ...]" "newsgroups to test"
#        ArgInt   count   "[c|count integer]"         "group repeat count"
#        ArgStr   dirname "[d|directory pathname]"    "working directory"
#        ArgBool  xflag   "[x|xflag]"                 "turn on x-mode"
#        ArgClear yflag   "[y|yflag]"                 "turn off y-mode"
#        ArgChar  sepch   "[s|separator char]"        "field separator"
#        ArgStr   files   "[f|files filename ...]"    "files to process"
#        ArgStr   name    "[n|name] name"             "name to use"
#        ArgStr   argv    "[arguments ...]"           "remaining arguments"
#     }
#
#     set count 1 ;    set dirname "." ;   set sepch "," ;
#     set xflag 0 ;    set yflag 1 ;
#     set files {} ;   set groups {} ;
#     set string "" ;
#
#     eval [ cmdparse -decls $arguments -- $scriptName $argv ]
#
###^^####
proc cmdparse args {
      ## set temp-file name
   if {( ! [info exists env(TMP)] )}  { set env(TMP) "/tmp" }
   if {( $env(TMP) == "" )}  { set env(TMP) "/tmp" }
   set tmpFileName "$env(TMP)/tmp[id process]"

       ## isolate the last argument (a list) from the rest
   set last [expr {[llength $args] - 1}]
   set cmdArgs [lindex $args $last]
   set cmdOpts [lrange $args 0 [expr {$last - 1}]]

      ## fork and exec
   if {( [set childPid [fork]] == 0 )} {
         ## This is the child ...
         ##    redirect stdout to temp-file and exec cmdparse(1)
         ##
      set tmpFile [open $tmpFileName "w"]
      close stdout
      dup $tmpFile stdout
      close $tmpFile
      execl cmdparse [concat -shell=tcl $cmdOpts $cmdArgs]
   } else {
         ## This is the parent ...
         ##    wait for the child, check its status, then return its output
         ##    dont forget to remove the temp-file.
         ##
      set childStatus [wait $childPid]
      set how [lindex $childStatus 1]
      set ret [lindex $childStatus 2]
      if {( ($how == "EXIT")  &&  ($ret == 0) )} {
         set variableSettings [exec cat $tmpFileName]
         unlink -nocomplain $tmpFileName
         return $variableSettings
      } else {
         unlink -nocomplain $tmpFileName
         exit [expr {$how == "EXIT" ? $ret : 127}]
      }
   }
}

