class Goose < Formula
  desc "A Cargo-inspired package manager and build tool for C"
  homepage "https://github.com/wess/goose"
  url "https://github.com/wess/goose/archive/v0.1.1.tar.gz"
  sha256 "899b430a698b56e99fb408ea083aac03b9d87b4a090c4d1ff8cbbc502155b43b"
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
