class Goose < Formula
  desc "A Cargo-inspired package manager and build tool for C"
  homepage "https://github.com/wess/goose"
  url "https://github.com/wess/goose/archive/v0.1.3.tar.gz"
  sha256 "1fc9f0bd6ce833765b7ad50a8bd1755eeab13e835d89a4b64ac7a64f0dc57f69"
  license "MIT"

  def install
    system "make", "clean"
    system "make"
    system "make", "install", "PREFIX=#{prefix}"
  end

  test do
    assert_match version.to_s, shell_output("#{bin}/goose --version")
  end
end
