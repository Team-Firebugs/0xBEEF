package BEEF;

use 5.014002;
use strict;
use warnings;

require Exporter;

our @ISA = qw(Exporter);
our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01';

require XSLoader;
XSLoader::load('BEEF', $VERSION);
1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

BEEF - Perl extension for managing packed radix tree + data

=head1 SYNOPSIS

  use BEEF;
  my $b = BEEF->new(0xBEEF);
  $b->store("key","value");
  my $value = $b->find("key");

  # see the README file or the tests for more examples
=head1 DESCRIPTION

see the README file

=head2 EXPORT

None by default.



=head1 SEE ALSO

=head1 AUTHOR

borislav nikolov, E<lt>jack@sofialondonmoskva.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2013 by borislav nikolov

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
