use strict;
use warnings;
use BEEF; 
use Test::More tests => 4;
use Data::Dumper;
my $val = 'a' x 100;

my $b = BEEF->new(0xBEEF);

$b->store("bzbz",$val);

is $b->find("bzbz"), $val;
is $b->find("bzbzz"), undef;

$b->reset();
is $b->find("bzbz"), undef;

eval {
    $b->store("AAA","a");
};
like $@, qr/unable to store/;

