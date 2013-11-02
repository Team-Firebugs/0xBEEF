use strict;
use warnings;
use BEEF; 
use Test::More tests => 2;
use Data::Dumper;
my $val = 'a' x 100;

my $b = BEEF->new(0xBEEF);

$b->store("bzbz",$val);
eval {
    $b->find_locally("bzbz");
};
like $@, qr/copy_locally/;
$b->copy_locally();
is $b->find_locally("bzbz"),$val;
