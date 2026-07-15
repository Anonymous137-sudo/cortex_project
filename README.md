# Cortex Project

Cortex Project is a school demonstration repository for an AI-assisted decentralized banking solution.

This repository packages a desktop client, backend node, wallet logic, and supporting communication features into a single Windows-friendly project for presentation and evaluation.

## Project Focus

- AI-assisted banking workflow concept
- decentralized account and wallet model
- secure transaction handling
- local desktop client plus backend service
- peer-to-peer communication layer
- Windows-ready release packaging for school lab systems

## Windows Demo Use

This repository is prepared primarily for Windows school PCs.

To run the packaged demo:

1. Open the GitHub Releases page.
2. Download `CryptEX_windows_x86_64_bundle.zip`.
3. Extract the zip fully into one folder.
4. Launch `cryptexqt_win32.exe`.
5. Keep all DLLs and the `plugins` folder beside the executable.

## Repository Layout

- `/src` - core backend, wallet, networking, mining, RPC, and storage logic
- `/gui` - desktop application
- `/website` - website and presentation-facing web assets
- `/scripts` - packaging and release helper scripts
- `/docs/WHITEPAPER_SCHOOL.docx` - school whitepaper

## Documentation

- School whitepaper: [`docs/WHITEPAPER_SCHOOL.docx`](./docs/WHITEPAPER_SCHOOL.docx)
- Technical notes: [`docs`](./docs)

## Release Policy For This Repo

This school-facing repository is configured so staged releases focus on the Windows runtime bundle used by the school lab environment.
