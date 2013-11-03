begin
      require 'beef'
rescue LoadError
      require './lib/beef'
end
require 'benchmark/ips'
n = 1_000_000
b = BEEF.create(0xbeef)
key = "abcdef"
missing = "abcdefg"
value = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
hash = {}
Benchmark.ips do |x|
    x.report("hash set") { hash[key] = value  }
    x.report("beef set") { b.store(key,value) }
    x.report("hash get") { z = hash[key] }
    x.report("beef get") { z = b.find(key) }
    x.report("hash mis") { z = hash[missing] }
    x.report("beef mis") { z = b.find(missing) }
end
