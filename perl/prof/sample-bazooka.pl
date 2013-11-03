use strict;
use warnings;
use BEEF; 
my $val = 'a' x 100;
my $b = BEEF->new(0xBEEF);

$b->store("bzbz",$val);
for (1..1_000_000) {
    my $x = $b->bazooka("bzbz");
}
