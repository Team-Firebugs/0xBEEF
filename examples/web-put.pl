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

set port => 3001;
get '/:key/:value' => sub {
   my $x = eval {
       $b->store(param('key'),param('value'));
       return "saved";
   } or do {
        return $@ || 'zombie';
   };
   return $x;
};
dance;
