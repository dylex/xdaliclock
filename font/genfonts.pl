#!/usr/bin/perl -w
# Copyright © 2010 Jamie Zawinski <jwz@jwz.org>
#
# Permission to use, copy, modify, distribute, and sell this software and its
# documentation for any purpose is hereby granted without fee, provided that
# the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  No representations are made about the suitability of this
# software for any purpose.  It is provided "as is" without express or 
# implied warranty.
#
# Created: 12-Jul-2010.

require 5;
use diagnostics;
use strict;

my $progname = $0; $progname =~ s@.*/@@g;
my $version = q{ $Revision: 0.00 $ }; $version =~ s/^[^\d]+([\d.]+).*/$1/;

# Target sizes:
#
#  

# Screen sizes:
#
# PalmOS:  160x160, 320x320, 320x480
#          128, 116, 65, 27
#          [not 208 104 91 58 45 28]
# WebOS:   320x480 (318x420)
#          302, 226, 131, 93, 78, 65
#          [not 208 91 58]
#          [not 207 90 58]
# iPhone:  320x480, 640x960
#          [not 417 208 183 117 91 58]
#   X11:   512, 256, 128, 72, 37, 27
# MacOS:   scales
#   GTK:   scales
#
# Existing heights:
#
#  ID  Char/Dash x Height  HHMMSS HHMM SS  OS
#  -------------------------------------------------------
#  A   705 / 387 x 1024    5004 3207 1410  
#  B   353 / 194 x 512     2506 1606  706  OSX, X11
#  B2  208 / 114 x 302     1476  946  416  WebOS
#  C   176 /  97 x 256     1250  801  352  OSX, X11, GTK
#  C2  156 /  85 x 226     1106  709  312  WebOS
#  C3   90 /  49 x 131      638  409  180  WebOS
#  D    88 /  48 x 128      624  400  176  OSX, X11, GTK, PalmOS
#  D2   78 /  42 x 116      552  354  156  PalmOS
#  D3   64 /  35 x 93       454  291  128  WebOS
#  D4   54 /  29 x 78       382  245  108  WebOS
#  2    64 /  17 x 72       418  273  128  X11
#  E    44 /  24 x 65       312  200   88  OSX, GTK, PalmOS, WebOS
#  1    30 /  11 x 37       202  131   60  X11
#  F    23 /  14 x 33       166  106   46  OSX, GTK
#  0    21 /   7 x 27       140  91    42  X11, PalmOS
#  G    12 /   7 x 16        86  55    24  GTK


my $verbose = 0;

sub safe_system(@) {
  my @cmd = @_;
    print STDERR "$progname: executing: " . join(' ', @cmd) . "\n"
      if ($verbose > 1);
  system (@cmd);
  my $exit_value  = $? >> 8;
  my $signal_num  = $? & 127;
  my $dumped_core = $? & 128;
  error ("$cmd[0]: core dumped!") if ($dumped_core);
  error ("$cmd[0]: signal $signal_num!") if ($signal_num);
  error ("$cmd[0]: exited with $exit_value!") if ($exit_value);
}


# Given a rectangle, picks the largest Dali Clock font that will fit in it.
#
sub pick_font_size($$$) {
  my ($mode, $w, $h) = @_;

  my $daspect = 705 / 1024;	# ratio of font's digit width to line height.
  my $caspect = 387 / 705;	# ratio of font's digit width to colon width.
  my $margin = 0.1;		# how much border space to leave.

  my ($numbers, $colons);
  if    ($mode eq 'HHMMSS') { ($numbers, $colons) = (6, 2); }
  elsif ($mode eq 'HHMM')   { ($numbers, $colons) = (4, 1); }
  elsif ($mode eq 'SS')     { ($numbers, $colons) = (2, 0); }
  else { error ("unknown mode $mode"); }

  # To compute the digit with for this line:
  #   colon_width = number_width * caspect
  #   w  = (numbers*number_width) + (colons*colon_width)
  # Simplifies to:
  #   w  = (numbers*number_width) + (colons*number_width*caspect)
  # Which is:
  #   A = B*C + D*C*E
  # Therefore:
  #   C = A/(B+D*E)
  #   number_width = w / (numbers + colons * caspect)

  my $number_width  = $w / ($numbers + $colons * $caspect);
  my $number_height = int ($number_width / $daspect);

  # But if that width makes the line too tall, shrink it.
  #
  if ($number_height > $h) {
    $number_width /= ($h / $number_height);
    $number_height = $h;
  }

  $number_height = int ($number_height * (1 - $margin));
  print STDERR "$progname: $w x $h, $mode: $number_height\n"
    if ($verbose);

  return $number_height;
}


sub pick_font_sizes(@) {
  my (@sizes) = @_;
  my %sizes;

  foreach my $size (@sizes) {
    my ($w, $h) = ($size =~ m/^(\d+)\s*x\s*(\d+)$/s);
    foreach my $mode ('HHMMSS', 'HHMM', 'SS') {
      my $fs = pick_font_size ($mode, $w, $h);
      $sizes{$fs} = 1;
    }
  }

  return sort { $b <=> $a} keys (%sizes);
}


sub image_size($) {
  my ($file) = @_;
  error ("$file does not exist") unless -f $file;
  return (0, 0) unless -f $file;
  my $cmd = ("identify -format %wx%h '$file'");
  print STDERR "$progname: executing: $cmd\n" if ($verbose > 2);
  my $result = `$cmd`;
  print STDERR "$progname:   ==> $result\n" if ($verbose > 2);
  my ($w, $h) = ($result =~ m/^(\d+)x(\d+)$/);
  error ("no size: $file") unless ($w && $h);
  return ($w, $h);
}


sub generate_font($) {
  my ($height) = @_;
  my $base_img = "dalifont.gif";

  my ($base_w, $base_h) = image_size ($base_img);

  $height = $base_h if ($height > $base_h);

  my $tmpbase = sprintf ("%s/font_%08x_", ($ENV{TMPDIR} || "/tmp"),
                         rand(0xFFFFFFFF));

  # Create "0.png" through "b.png", the individual characters.
  #
  my @cmd = ("convert",
             $base_img,
             "-filter", "box",		# Better for reduction of these.
             "-resize", "x$height",	# Shrink to target height.
             "-threshold", "33%",	# Monochrome.
             "-crop", "12x1@",		# Each character on a canvas.
             "+adjoin",
             "+repage",
             "${tmpbase}%x.png",
            );
  safe_system (@cmd);

  for (my $c = 0; $c < 12; $c++) {
    my $in  = sprintf ("${tmpbase}%x.png", $c);
    my $out = sprintf ("%x_$height.xbm", $c);
    @cmd = ("convert",
            $in,

            "-filter", "box",		# Better for reduction of these.
            "-resize", "x$height",	# Shrink to target height.

            # Protect left/top/bottom from trimming, and trim right.
            "-gravity", "west",
            "-background", "black", "-splice", "1x0",
            "-background", "white", "-splice", "1x0",
            "-trim", "+repage",       "-chop", "1x0",

            # Protect right/top/bottom from trimming, and trim left.
            "-gravity", "east",
            "-background", "black", "-splice", "1x0",
            "-background", "white", "-splice", "1x0",
            "-trim", "+repage",       "-chop", "1x0",

#            "+dither",
#            "-monochrome",
#            "-threshold", "1",
             "$out",
            );
    safe_system (@cmd);
    unlink ($in);
    safe_system ("identify $out");
  }

  exit 0;
}


sub generate_fonts(@) {
  my (@sizes) = @_;
  foreach my $s (pick_font_sizes (@sizes)) {
    print STDOUT "$s\n";
#    generate_font ($s);
  }
}


sub error($) {
  my ($err) = @_;
  print STDERR "$progname: $err\n";
  exit 1;
}

sub usage() {
  print STDERR "usage: $progname [--verbose] sizes ...\n";
  exit 1;
}

sub main() {
  my @sizes = ();
  while ($#ARGV >= 0) {
    $_ = shift @ARGV;
    if (m/^--?verbose$/) { $verbose++; }
    elsif (m/^-v+$/) { $verbose += length($_)-1; }
    elsif (m/^-./) { usage; }
    elsif (m/^(\d+)\s*x\s*(\d+)$/s) { push @sizes, $_; }
    else { usage; }
  }
  usage unless ($#sizes >= 0);
  generate_fonts (@sizes);
}

main();
exit 0;
