Gem::Specification.new do |s|
    s.name         = "beef"
    s.version      = "0.0.1"
    s.summary      = "beef - shared memory key/value store"
    s.description  = "beef - shared memory key/value store (using binary semaphore as mutex)"
    s.homepage     = "https://github.com/jackdoe/0xBEEF"
    s.authors      = ["Borislav Nikolov"]
    s.email        = "jack@sofialondonmoskva.com"
    s.files        = Dir.glob("ext/**/*.{c,h,rb}") + Dir.glob("lib/**/*.rb")
    s.extensions << "ext/beef/extconf.rb"
    s.add_development_dependency "rake-compiler"
end
