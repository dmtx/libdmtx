#!/usr/bin/env ruby
require 'rubygems'
require 'RMagick'

require 'Rdmtx'

rdmtx = Rdmtx.new

if ARGV[0]
  image = Magick::Image.read(ARGV[0]).first
  puts "The image contains : "
  puts rdmtx.decode(image, 0)
else
  rdmtx.encode("Hello you !!").write("output.png")
  puts "Written output.png"
end
