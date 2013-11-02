use strict;
use warnings;
use BEEF; 
use Test::More;
use Data::Dumper;

my $b = BEEF->new(0xBEEF);
my $val = 'a' x 100;
for my $i(1..100) {
    $b->store('x'x$i,$val . $i);
    is $b->find('x'x$i), $val . $i;
}

my $info = $b->info();
my $exported = $b->export();
$b->reset();

for my $i(1..100) {
    is $b->find("x"x$i), undef;
}

$b->overwrite($exported);
for my $i(1..100) {
    is $b->find('x'x$i), $val . $i;
}

is $b->info(), $info;
done_testing()
