# encoding: utf-8
require 'test/unit'
require ENV['USE_CURRENT_DIRECTORY'] ? File.absolute_path(File.join(File.dirname(__FILE__),'..','lib','beef')) : 'beef'

class Test::Unit::TestCase
    def test_everything
        x = BEEF.create(0xbeef);
        x.store("key","value")
        assert_equal x.find("key"), "value"

        assert_raise(ArgumentError) do
            BEEF.create(0xbeef + 1)
        end
        raw = x.export

        x1 = BEEF.create(0xbeef + 2)
        x1.overwrite(raw)
        assert_equal x1.find("key"),"value"
        x = nil
    end
end
