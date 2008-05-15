package MCaster::Server;

use strict;
use Apache::MP3::Resample;
use Apache::Constants qw(:common REDIRECT HTTP_NO_CONTENT DIR_MAGIC_TYPE HTTP_NOT_MODIFIED);
use CGI qw/:standard *TR param/;
use File::Basename 'dirname','basename';
use File::Path;
use vars qw(@ISA $VERSION @EXPORT);

@ISA     = qw(Apache::MP3::Resample);
@EXPORT  = qw(read_library read_playlists get_mp3 read_directory);
$VERSION = 1.60;

my %mp3s;
my %plsts;

my $MyName           = "MediaCaster";

my $GroupsFileVar    = "GroupsFile";
my $AdminGroupVar    = "AdminGroup";
my $DwnldGroupVar    = "DownloadGroup";
my $LibraryFileVar   = "LibraryFile";
my $PlaylistsFileVar = "PlaylistsFile";

my $LibraryFile      = "/usr/MCaster/htdocs/library.txt";
my $PlaylistsFile    = "/usr/MCaster/htdocs/playlists.txt";
my $GroupsFile       = "/usr/MCaster/conf/groups";
my $AdminGroup       = "admin";
my $DwnldGroup       = "download";
my $FIXME            = "/mcaster/web/";

# Pre-load the groups with the admin
my %groups         = (admin    =>qw(admin),
                      download =>qw(admin));

sub read_vars {
   my $self   = shift;
   my $r      = $self->r;
   my $config = $r->dir_config;

   $GroupsFile    = $r->dir_config($GroupsFileVar)    if($r->dir_config($GroupsFileVar));
   $AdminGroup    = $r->dir_config($AdminGroupVar)    if($r->dir_config($AdminGroupVar));
   $DwnldGroup    = $r->dir_config($DwnldGroupVar)    if($r->dir_config($DwnldGroupVar));
   $LibraryFile   = $r->dir_config($LibraryFileVar)   if($r->dir_config($LibraryFileVar));
   $PlaylistsFile = $r->dir_config($PlaylistsFileVar) if($r->dir_config($PlaylistsFileVar));

#  $r->log_error("Groups File:    $GroupsFile");
#  $r->log_error("Admin Group:    $AdminGroup");
#  $r->log_error("Download Group: $DwnldGroup");
#  $r->log_error("Library File:   $LibraryFile");
#  $r->log_error("Playlists File: $PlaylistsFile");
}


sub get_mp3 {
   my $file    = $_[0];
   my $hashref = $mp3s{$file};

   return $hashref;
}


# read a single directory, returning lists of subdirectories and MP3 files
sub read_directory {
   my $self      = shift;
   my $dir       = shift;
   my @playlists = keys(%plsts);
   my @directories;
   my @txtfiles;

   for my $d (keys(%mp3s)) {
      my $data  = get_mp3($d);
      $mp3s{$d} = $data;

      unless ($self->read_cache("$dir/$d")) {
         $self->write_cache("$dir/$d" => $data);
      }
   }

   return \(@directories,%mp3s,@playlists,@txtfiles);
}


sub download_ok {
   my $self = shift;
   my $r    = $self->r;
   my $conn = $r->connection;
   my $auth = $conn->auth_type;
   my $user = $conn->user;

   if ($self->Apache::MP3::download_ok()) {
      read_users($r);
      my $tmp     = $groups{$DwnldGroupVar};
      my @dwnldrs = @$tmp;
      for my $dwnldr (@dwnldrs) {
         if ($dwnldr eq $user) {
#           $r->log_error("Allowing download: '$user'");
            return 1;
         }
      }
   }
   return 0;
}


# format a playlist entry and return its HTML
sub format_playlist {
  my $self     = $_[0];
  my $playlist = $plsts{$_[1]};
  my $title    = $playlist->{name};
  my $url      = $playlist->{url} . "?play=1";
  return p(
			a({-href => $url},
             	img({-src => $self->playlist_icon,
                     -align  => 'ABSMIDDLE',
                     -class  => 'subdir',
                     -alt    => $self->x('Playlist'),
                     -border => 0}))
           		. "&nbsp;" .
           	a({-href  => $url},
           		font({-class => 'subdirectory'},
                	$title)));
}


sub read_library {
my $LibraryFile = "/usr/MCaster/htdocs/library.txt";
my $FIXME       = "/mcaster/web/";
   open(LIBR,$LibraryFile);
   while (<LIBR>) {
      chop;
      my $line = $_;
      my @cols = split(/\|/,$line);
      my %flds;
   
      # Coerce it back into a relative file path
      $_ = $cols[7];
      s|$FIXME||g;
      s|%20| |g;
      s|%23|#|g;
      my $file = $_;

      $flds{title}    = $cols[0];
      $flds{artist}   = $cols[1];
      $flds{album}    = $cols[2];
      $flds{genre}    = $cols[3];
      $flds{year}     = $cols[4];
      $flds{track}    = $cols[5];
      $flds{seconds}  = $cols[6];
      $flds{filename} = $file;
      $flds{comment}  = $cols[8];

      $flds{sec}      = int($flds{seconds}%60);
      $flds{min}      = int($flds{seconds}/60);
      $flds{duration} ="$flds{min}:$flds{sec}";

      $mp3s{$file} = \%flds;
   }

   close LIBR;
   return \%mp3s;
}


sub read_playlists {
my $PlaylistsFile = "/usr/MCaster/htdocs/playlists.txt";
my $FIXME         = "/mcaster/web/";
   open(PLAY,$PlaylistsFile);
   while (<PLAY>) {
      chop;
      my $line = $_;
      my ($name,$type,$file) = split(/\|/,$line);
      if($type && $type eq "m3u") {
         my $url  = "$FIXME$file";
         my %plst = (name => $name, url => $url);
         $plsts{$url} = \%plst;
      }
   }
   close(PLAY);
}


sub read_users {
   open(GRPS,$GroupsFile);
   while (<GRPS>) {
      chop;
      my @cols  = split(":",$_);
      my $group = $cols[0];
      my @list  = split(" ",$cols[1]);

      $groups{$AdminGroupVar} = \@list if ($group eq $AdminGroup);
      $groups{$DwnldGroupVar} = \@list if ($group eq $DwnldGroup);
   }
   close GRPS;
}


sub list_directory {
   my $self = shift;
   my $dir  = shift;

   return DECLINED unless -d $dir;

   my $last_modified     = (stat($LibraryFile  ))[9];
   my $ply_last_modified = (stat($PlaylistsFile))[9];

   $last_modified = $ply_last_modified if ($ply_last_modified>$last_modified);

   $self->r->header_out('ETag' => sprintf("%lx-%s", $last_modified, $VERSION));

   if (my $check = $self->r->header_in("If-None-Match")) {
      my ($time, $ver) = $check =~ /^([a-f0-9]+)-([0-9.]+)$/;

      if ($check eq '*' or (hex($time) == $last_modified and $ver == $VERSION)) {
         return HTTP_NOT_MODIFIED;
      }
   }

   return DECLINED unless my ($directories,$mp3s,$playlists,$txtfiles)
      = $self->read_directory($dir);

   $self->r->send_http_header( $self->html_content_type );
   return OK if $self->r->header_only;

   $self->page_top($dir);
   $self->directory_top($dir);
   if(@$directories) {
      $self->list_subdirs($directories);
   }
   if(@$txtfiles) {
      $self->list_txtfiles($txtfiles);
   }
   if(@$playlists) {
      $self->list_playlists($playlists);
   }
   if(%$mp3s) {
      $self->list_mp3s($mp3s);
   }
   print hr                         unless %$mp3s;
   $self->directory_bottom($dir);
   return OK;
}


sub page_top {
   my $self  = shift;
   my $dir   = shift;
   my $title = "$MyName";

   print start_html(
      -title => $title,
      -head  => meta({-http_equiv => 'Content-Type',
                      -content    => 'text/html; charset='
                                    . $self->html_content_type
                    }),
      -lang  => $self->lh->language_tag,
      -dir   => $self->lh->direction,
      -style => {-src=>$self->stylesheet}
   );

}


sub directory_top {
   my $self = shift;
   my $dir  = shift;

   print start_table({-width=>'100%'});
      print start_TR;
         print start_td;
            print font({-class => 'directory',
                        -size  => '+2'}, $MyName);
         print end_td;
      print end_TR;

      print start_TR;
         print start_td;
            print a({-href => './playlist.m3u?Play+All+Recursive=1'},
	                img({-src   => $self->cd_icon($dir), $self->aleft,
                         -alt   => $self->x('Stream All'),
	                     -border=> 0}));

            print a({-href => './playlist.m3u?Shuffle+All+Recursive=1'},
	                font({-class => 'directory'},
                    '[', $self->x('Shuffle All'), ']'));
   
            print '&nbsp;';

            print a({-href => './playlist.m3u?Play+All+Recursive=1'},
	                font({-class => 'directory'},
                    '[', $self->x('Stream All'), ']'));
         print end_td;

         print td({-align=>'RIGHT',-valign=>'TOP'},
                  $self->sample_popup());
      print end_TR;
  print end_table;
}


# A bug in here WRT ports
sub stream_base {
  my $self = shift;
  my $suppress_auth = shift;
  my $r = $self->r;

  my $auth_info;
  # the check for auth_name() prevents an annoying message in
  # the apache server log when authentication is not in use.
  if ($r->auth_name && !$suppress_auth) {
    my ($res,$pw) = $r->get_basic_auth_pw;
    if ($res == 0) { # authentication in use
      my $user = $r->connection->user;
      $auth_info = "$user:$pw\@";
    }
  }

  if ((my $basename = $r->dir_config('StreamBase')) && !$self->is_localnet()) {
    $basename =~ s!http://!http://$auth_info! if $auth_info;
    return $basename;
  }

  my $vhost = $r->hostname;
  unless ($vhost) {
    $vhost = $r->server->server_hostname;
  }

  # Here it is, this was in the block above
  $vhost .= ':' . $r->get_server_port unless $r->get_server_port == 80;
  return "http://${auth_info}${vhost}";
}


sub stream {
   my $self = shift;
   my $r    = $self->r;
   my $file = $r->filename;
   my $uri  = $r->uri;
   my $base = $self->stream_base;

   return DECLINED unless -e $r->filename;  # should be $r->finfo

   unless ($self->stream_ok) {
      $r->log_reason('AllowStream forbidden');
      return FORBIDDEN;
   }

   if ($self->check_stream_client and !$self->is_stream_client) {
      my $useragent = $r->header_in('User-Agent');
      $r->log_reason("CheckStreamClient is true and $useragent is not a streaming client");
      return FORBIDDEN;
   }

   return $self->send_stream($r->filename,$r->uri);
}


sub find_mp3s {
   my @uris;
   for my $uri (keys(%mp3s)) {
      push(@uris, $FIXME.$uri);
   }
   return \@uris;
}


sub run {
  my $self = shift;
  my $r    = $self->r;

  local $CGI::XHTML = 0;

  # check that we aren't running under PerlSetupEnv Off
  if ($ENV{MOD_PERL} && !$ENV{SCRIPT_FILENAME}) {
     warn "CGI.pm cannot run with 'PerlSetupEnv Off', please set it to On";
  }

  # this is called to show a help screen
  return $self->help_screen if param('help_screen');

  # generate directory listing
  return $self->process_directory($r->filename) 
    if -d $r->filename;  # should be $r->finfo, but STILL problems with this

  #simple download of file
  return $self->download_file($r->filename) unless param;

  # this is called to stream a file
  return $self->stream if param('stream');

  # this is called to generate a playlist on the current directory
  return $self->send_playlist($self->find_mp3s)
    if param('Play All');

  # this is called to generate a playlist on the current directory
  # and everything beneath
  return $self->send_playlist($self->find_mp3s('recursive')) 
    if param('Play All Recursive') ;

  # this is called to generate a shuffled playlist of current directory
  return $self->send_playlist($self->find_mp3s,'shuffle')
    if param('Shuffle');

  # this is called to generate a shuffled playlist of current directory
  return $self->send_playlist($self->find_mp3s,'shuffle')
    if param('Shuffle All');

  # this is called to generate a shuffled playlist of current directory
  # and everything beneath
  return $self->send_playlist($self->find_mp3s('recursive'),'shuffle')
    if param('Shuffle All Recursive');

  # this is called to generate a playlist for one file
  if (param('play')) {
    my $dot3 = '.m3u|.pls';
    my($basename) = $r->uri =~ m!([^/]+?)($dot3)?$!;
    $basename = quotemeta($basename);
    my @matches;

    # This is called for one our playlist files
    if ($plsts{$r->uri}) {
       my $plst = $r->uri;
       $plst =~ s|$FIXME|/usr/MCaster/htdocs/|;
       @matches = $self->load_playlist($plst);
       for my $match (@matches) {
          $match =~ s|$FIXME||;
          $match =~ s|%20| |g;
          $match =~ s|%23|#|g;
       }

    # This is called when the playlist file lives in content dir
    } elsif (-e $self->r->filename) {
      @matches = $self->load_playlist($self->r->filename);

    # Psuedo playlist really reference the file
    } else {
      # find the MP3 file that corresponds to basename.m3u
      @matches = grep { m!/$basename[^/]*$! } @{$self->find_mp3s};
    }

    if($r->request($r->uri)->content_type eq 'audio/x-scpls') {
      open(FILE,$r->filename) || return 404;
      $r->send_fd(\*FILE);
      close(FILE);
    } else {
      $self->send_playlist(\@matches);
    }

    return OK;
  }

  # this is called to generate a playlist for selected files
  if (param('Play Selected')) {
    return HTTP_NO_CONTENT unless my @files = param('file');
    my $uri = dirname($r->uri);
    $self->send_playlist([map { "$uri/$_" } @files]);
    return OK;
  }

  # otherwise don't know how to deal with this
  $self->r->log_reason('Invalid parameters -- possible attempt to circumvent checks.');
  return FORBIDDEN;
}


BEGIN {
   read_library();
   read_playlists();
}
