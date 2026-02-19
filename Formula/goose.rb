class Goose < Formula
  desc "A Cargo-inspired package manager and build tool for C"
  homepage "https://github.com/wess/goose"
  url "https://github.com/wess/goose/archive/v0.1.0b.tar.gz"
  sha256 "SKIP"
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
