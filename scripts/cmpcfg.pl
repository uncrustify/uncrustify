#!/usr/bin/perl
#
# Copyright (c) 2006 David Thompson
# da.thompson@yahoo.com
# Fri Nov 17 20:41:23 PST 2006
# License: GPL

# Purpose of this script is to process config files and
# produce a comparision chart of values.  The input files
# are simple series of parameter definitions, of the form
# 'name=value' pairs, whitespace and comments are correctly
# ignored.  Invoke on multiple config files to compare
# parameter values for all files, try this,
#   cd /usr/local/share/uncrustify
#   cmpcfg.pl *.cfg

# first build hashes from all input files
# 1. %name is a master hash of all parameter names found
#    across all files, we use a hash to remember the keys,
#    we don't compare about the values stored for each key
# 2. %table is a per file 2 dimensional hash array indexed
#    by the current filename and parameter; ie, this hash
#    stores the 'name=value' pairs on per file basis
foreach my $file (@ARGV) {
    open FH, "<$file"
        or die "Can't open file: $file";
    while (<FH>) {
        chomp;
        next if (/^[ \t]*$/);            # ignore blank lines
        next if (/^[ \t]*#/);            # ignore comment lines
        s/#.*$//;                        # strip trailing comments
        s/^[ \t]*//;                     # strip leading whitespace
        s/[ \t]*$//;                     # strip trailing whitespace
        s/[ \t]*=[ \t]*/=/;              # remove whitespace around '='
        $_ = lc;                         # lowercase everything
        ($name, $value) = split /=/;     # extract name and value
        $names{$name} = $name;           # master hash of all names
        $table{$file}{$name} = $value;   # per file hash of names
    }
    close FH;
}

# find longest parameter name
# we'll use this later for report printing
foreach $name (sort keys %names) {
    if (length($name) > $maxlen) {
        $maxlen = length($name);
    }
}
$maxlen += 4;  # add extra padding

# return string centered in specified width
sub center {
    ($wid, $str) = @_;
    $flg = 0;
    while (length($str) < $wid) {
        if ($flg) {
            $flg = 0;
            $str = " " . $str;
        } else {
            $flg = 1;
            $str = $str . " ";
        }
    }
    return $str;
}

# print legend for filenames
$cnt = 0;
foreach $file (@ARGV) {
    $cnt++;
    print " <$cnt>  $file\n";
}

# blank line separates legend & header
print "\n";

# print header line
print " " x $maxlen . " ";
$cnt = 0;
foreach (@ARGV) {
    $cnt++;
    $fmt = "<$cnt>";
    print "     ".&center(6, $fmt);
}
print "\n";

# print body of report, one line per parameter name
foreach $name (sort keys %names) {
    printf "%-*s ", $maxlen, $name;
    foreach $file (@ARGV) {
        if (defined($table{$file}{$name})) {
            print "     ".&center(6, $table{$file}{$name});
        } else {
            # parameter not defined for this file
            print "     ".&center(6, "*");
        }
    }
    print "\n";
}

