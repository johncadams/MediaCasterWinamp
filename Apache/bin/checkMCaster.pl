#!/usr/bin/perl -w

use Apache::MP3::MCaster;

read_directory();
my $tmp  = read_library();
my @uris = @$tmp;

read_playlists();
die;

for my $uri (@uris) {
printf("uri:%s\n",$uri);
   my $tmp  = get_mp3($uri);
   my %mp3  = %$tmp;
   printf("mp3:%s\n",join(",",%mp3));
}
