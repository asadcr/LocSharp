sub FETCH {
    my ($self, $extra) = @_;
    return bless ref($self)->new(@$self, $extra), ref($self);
}
my %imports = map {$_ => "Regexp::Common::$_"}
              qw /balanced CC     comment   delimited lingua list
                  net      number profanity SEN       URI    whitespace
                  zip/;
sub import {
    shift;  # Shift off the class.
    tie %RE, __PACKAGE__;
    {
        no strict 'refs';
        *{caller() . "::RE"} = \%RE;
    }
    my $saw_import;
    my $no_defaults;
    my %exclude;
    foreach my $entry (grep {!/^RE_/} @_) {
        if ($entry eq 'pattern') {
            no strict 'refs';
            *{caller() . "::pattern"} = \&pattern;
            next;
        }
        # This used to prevent $; from being set. We still recognize it,
        # but we won't do anything.
        if ($entry eq 'clean') {
            next;
        }
        if ($entry eq 'no_defaults') {
            $no_defaults ++;
            next;
        }
        if (my $module = $imports {$entry}) {
            $saw_import ++;
            eval "require $module;";
            die $@ if $@;
            next;
        }
        if ($entry =~ /^!(.*)/ && $imports {$1}) {
            $exclude {$1} ++;
            next;
        }
        # As a last resort, try to load the argument.
        my $module = $entry =~ /^Regexp::Common/
                            ? $entry
                            : "Regexp::Common::" . $entry;
        eval "require $module;";
        die $@ if $@;
    }
    unless ($saw_import || $no_defaults) {
        foreach my $module (values %imports) {
            next if $exclude {$module};
            eval "require $module;";
            die $@ if $@;
        }
    }
    my %exported;
    foreach my $entry (grep {/^RE_/} @_) {
        if ($entry =~ /^RE_(\w+_)?ALL$/) {
            my $m  = defined $1 ? $1 : "";
            my $re = qr /^RE_${m}.*$/;
            while (my ($sub, $interface) = each %sub_interface) {
                next if $exported {$sub};
                next unless $sub =~ /$re/;
                {
                    no strict 'refs';
                    *{caller() . "::$sub"} = $interface;
                }
                $exported {$sub} ++;
            }
        }
        else {
            next if $exported {$entry};
            _croak "Can't export unknown subroutine &$entry"
                unless $sub_interface {$entry};
            {
                no strict 'refs';
                *{caller() . "::$entry"} = $sub_interface {$entry};
            }
            $exported {$entry} ++;
        }
    }
}
sub AUTOLOAD { _croak "Can't $AUTOLOAD" }
sub DESTROY {}
my %cache;
my $fpat = qr/^(-\w+)/;
sub _decache {
        my @args = @{tied %{$_[0]}};
        my @nonflags = grep {!/$fpat/} @args;
        my $cache = get_cache(@nonflags);
        _croak "Can't create unknown regex: \$RE{"
            . join("}{",@args) . "}"
                unless exists $cache->{__VAL__};
        _croak "Perl $] does not support the pattern "
            . "\$RE{" . join("}{",@args)
            . "}.\nYou need Perl $cache->{__VAL__}{version} or later"
                unless ($cache->{__VAL__}{version}||0) <= $];
        my %flags = ( %{$cache->{__VAL__}{default}},
                      map { /$fpat\Q$;\E(.*)/ ? ($1 => $2)
                          : /$fpat/           ? ($1 => undef)
                          :                     ()
                          } @args);
        $cache->{__VAL__}->_clone_with(\@args, \%flags);
}
use overload q{""} => \&_decache;
sub get_cache {
        my $cache = \%cache;
        foreach (@_) {
                $cache = $cache->{$_}
                      || ($cache->{$_} = {});
        }
        return $cache;
}
sub croak_version {
        my ($entry, @args) = @_;
}
sub pattern {
        my %spec = @_;
        _croak 'pattern() requires argument: name => [ @list ]'
                unless $spec{name} && ref $spec{name} eq 'ARRAY';
        _croak 'pattern() requires argument: create => $sub_ref_or_string'
                unless $spec{create};
        if (ref $spec{create} ne "CODE") {
                my $fixed_str = "$spec{create}";
                $spec{create} = sub { $fixed_str }
        }
        my @nonflags;
        my %default;
        foreach ( @{$spec{name}} ) {
                if (/$fpat=(.*)/) {
                        $default{$1} = $2;
                }
                elsif (/$fpat\s*$/) {
                        $default{$1} = undef;
                }
                else {
                        push @nonflags, $_;
                }
        }
        my $entry = get_cache(@nonflags);
        if ($entry->{__VAL__}) {
                _carp "Overriding \$RE{"
                   . join("}{",@nonflags)
                   . "}";
        }
        $entry->{__VAL__} = bless {
                                create  => $spec{create},
                                match   => $spec{match} || \&generic_match,
                                subs    => $spec{subs}  || \&generic_subs,
                                version => $spec{version},
                                default => \%default,
                            }, 'Regexp::Common::Entry';
        foreach (@nonflags) {s/\W/X/g}
        my $subname = "RE_" . join ("_", @nonflags);
        $sub_interface{$subname} = sub {
                push @_ => undef if @_ % 2;
                my %flags = @_;
                my $pat = $spec{create}->($entry->{__VAL__},
                               {%default, %flags}, \@nonflags);
                if (exists $flags{-keep}) { $pat =~ s/\Q(?k:/(/g; }
                else { $pat =~ s/\Q(?k:/(?:/g; }
                return exists $flags {-i} ? qr /(?i:$pat)/ : qr/$pat/;
        };
        return 1;
}
sub generic_match {$_ [1] =~  /$_[0]/}
sub generic_subs  {$_ [1] =~ s/$_[0]/$_[2]/}
sub matches {
        my ($self, $str) = @_;
        my $entry = $self -> _decache;
        $entry -> {match} -> ($entry, $str);
}
sub subs {
        my ($self, $str, $newstr) = @_;
        my $entry = $self -> _decache;
        $entry -> {subs} -> ($entry, $str, $newstr);
        return $str;
}
package Regexp::Common::Entry;
# use Carp;
use overload
    q{""} => sub {
        my ($self) = @_;
        my $pat = $self->{create}->($self, $self->{flags}, $self->{args});
        if (exists $self->{flags}{-keep}) {
            $pat =~ s/\Q(?k:/(/g;
        }
        else {
            $pat =~ s/\Q(?k:/(?:/g;
        }
        if (exists $self->{flags}{-i})   { $pat = "(?i)$pat" }
        return $pat;
    };
sub _clone_with {
    my ($self, $args, $flags) = @_;
    bless { %$self, args=>$args, flags=>$flags }, ref $self;
}
1;
#
# This software is Copyright (c) 2001 - 2011, Damian Conway and Abigail.
#
# This module is free software, and maybe used under any of the following
# licenses:
#
#  1) The Perl Artistic License.     See the file COPYRIGHT.AL.
#  2) The Perl Artistic License 2.0. See the file COPYRIGHT.AL2.
#  3) The BSD Licence.               See the file COPYRIGHT.BSD.
#  4) The MIT Licence.               See the file COPYRIGHT.MIT.
EOCommon
# 2}}}
$Regexp_Common_Contents{'Common/comment'} = 'EOC';   # {{{2
package Regexp::Common::comment;
use Regexp::Common qw /pattern clean no_defaults/;
use strict;
use warnings;
use vars qw /$VERSION/;
$VERSION = '2010010201';
my @generic = (
    {languages => [qw /ABC Forth/],
     to_eol    => ['\\\\']},   # This is for just a *single* backslash.
    {languages => [qw /Ada Alan Eiffel lua/],
     to_eol    => ['--']},
    {languages => [qw /Advisor/],
     to_eol    => ['#|//']},
    {languages => [qw /Advsys CQL Lisp LOGO M MUMPS REBOL Scheme
                       SMITH zonefile/],
     to_eol    => [';']},
    {languages => ['Algol 60'],
     from_to   => [[qw /comment ;/]]},
    {languages => [qw {ALPACA B C C-- LPC PL/I}],
     from_to   => [[qw {/* */}]]},
    {languages => [qw /awk fvwm2 Icon m4 mutt Perl Python QML
                       R Ruby shell Tcl/],
     to_eol    => ['#']},
    {languages => [[BASIC => 'mvEnterprise']],
     to_eol    => ['[*!]|REM']},
    {languages => [qw /Befunge-98 Funge-98 Shelta/],
     id        => [';']},
    {languages => ['beta-Juliet', 'Crystal Report', 'Portia', 'Ubercode'],
     to_eol    => ['//']},
    {languages => ['BML'],
     from_to   => [['<?_c', '_c?>']],
    },
    {languages => [qw /C++/, 'C#', qw /AspectJ Cg ECMAScript FPL Java JavaScript JSX Stylus/],
     to_eol    => ['//'],
     from_to   => [[qw {/* */}]]},
    {languages => [qw /CLU LaTeX slrn TeX/],
     to_eol    => ['%']},
    {languages => [qw /False/],
     from_to   => [[qw !{ }!]]},
    {languages => [qw /Fortran/],
     to_eol    => ['!']},
    {languages => [qw /Haifu/],
     id        => [',']},
    {languages => [qw /ILLGOL/],
     to_eol    => ['NB']},
    {languages => [qw /INTERCAL/],
     to_eol    => [q{(?:(?:PLEASE(?:\s+DO)?|DO)\s+)?(?:NOT|N'T)}]},
    {languages => [qw /J/],
     to_eol    => ['NB[.]']},
    {languages => [qw /JavaDoc/],
     from_to   => [[qw {/** */}]]},
    {languages => [qw /Nickle/],
     to_eol    => ['#'],
     from_to   => [[qw {/* */}]]},
    {languages => [qw /Oberon/],
     from_to   => [[qw /(* *)/]]},
    {languages => [[qw /Pascal Delphi/], [qw /Pascal Free/], [qw /Pascal GPC/]],
     to_eol    => ['//'],
     from_to   => [[qw !{ }!], [qw !(* *)!]]},
    {languages => [[qw /Pascal Workshop/]],
     id        => [qw /"/],
     from_to   => [[qw !{ }!], [qw !(* *)!], [qw !/* */!]]},
    {languages => [qw /PEARL/],
     to_eol    => ['!'],
     from_to   => [[qw {/* */}]]},
    {languages => [qw /PHP/],
     to_eol    => ['#', '//'],
     from_to   => [[qw {/* */}]]},
    {languages => [qw !PL/B!],
     to_eol    => ['[.;]']},
    {languages => [qw !PL/SQL!],
     to_eol    => ['--'],
     from_to   => [[qw {/* */}]]},
    {languages => [qw /Q-BAL/],
     to_eol    => ['`']},
    {languages => [qw /Smalltalk/],
     id        => ['"']},
    {languages => [qw /SQL/],
     to_eol    => ['-{2,}']},
    {languages => [qw /troff/],
     to_eol    => ['\\\"']},
    {languages => [qw /vi/],
     to_eol    => ['"']},
    {languages => [qw /*W/],
     from_to   => [[qw {|| !!}]]},
    {languages => [qw /ZZT-OOP/],
     to_eol    => ["'"]},
);
my @plain_or_nested = (
   [Caml         =>  undef,       "(*"  => "*)"],
   [Dylan        =>  "//",        "/*"  => "*/"],
   [Haskell      =>  "-{2,}",     "{-"  => "-}"],
   [Hugo         =>  "!(?!\\\\)", "!\\" => "\\!"],
   [SLIDE        =>  "#",         "(*"  => "*)"],
  ['Modula-2'    =>  undef,       "(*"  => "*)"],
  ['Modula-3'    =>  undef,       "(*"  => "*)"],
);