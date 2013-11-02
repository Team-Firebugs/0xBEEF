use strict;
use warnings;
use BEEF; 
use Test::More;

my $b = BEEF->new(0xBEEF);
my $val = 'a' x 100;
for my $i(1..100) {
    $b->store('x'x$i,$val . $i);
    is $b->find('x'x$i), $val . $i;
}
my $file = "/tmp/exported:$$.delete_if_seen";
my $info = $b->info();
my $exported = $b->export();

open (my $fh, ">$file") or die $!;
print $fh $exported;
close($fh);


my $c = BEEF->new(0xBEF1);
my $val2 = 'b' x 50;
for my $i(1..100) {
    $c->store('x'x$i,$val2 . $i);
}

for my $i(1..100) {
    is $c->find('x'x$i), $val2 . $i;
}
isnt $b->info(), $c->info();

$c->reset();
open($fh,"<:raw",$file) or die $!;
my $blob = do { local $/; <$fh>; };
close($fh);
$c->overwrite($blob);


for my $i(1..100) {
    is $c->find('x'x$i), $val . $i;
}
is $c->info(),$b->info();

unlink($file) or die $!;
done_testing()
