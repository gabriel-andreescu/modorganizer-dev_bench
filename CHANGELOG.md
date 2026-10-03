# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/2.0.0/).

## [Unreleased]

## [0.1.1] - 2026-10-03

### Fixed

- The bridge skips discovery records left by MO2 processes that no longer run,
  and the plugin removes them when it starts. Probing them could make a running
  MO2 miss the discovery timeout and be reported as not running.

## [0.1.0] - 2026-09-30

### Added

- Initial release
