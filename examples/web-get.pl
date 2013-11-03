#!/usr/bin/env perl
use Dancer;
use BEEF;
my $b = BEEF->new(0xBEEF);


# so BEEF's destructor will be called to cleanup the 
# shared memory and semaphore
$SIG{INT} = sub {
    $b = undef;
    exit();
};

get '/:key' => sub {
  my $x = $b->find(param('key'));
  return $x || "not found";
};
dance;
