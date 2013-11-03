just some benchmarks compairing BEEF lookup to hash lookup in ruby

    hash set  3088253.2 (±3.1%) i/s -   15410647 in   4.994880s
    beef set  1289254.2 (±0.8%) i/s -    6448584 in   5.002095s
    hash get  4010468.1 (±0.7%) i/s -   20083472 in   5.008050s
    beef get   901228.6 (±16.0%) i/s -    4407501 in   5.034593s
    hash mis  4291381.9 (±0.6%) i/s -   21466170 in   5.002366s
    beef mis  1373213.8 (±1.1%) i/s -    6894200 in   5.021121s

