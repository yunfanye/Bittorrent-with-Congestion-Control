/* This is the readme file for 15441 project 2: Bittorrent */

We implement flow control, congestion control scheme on BitTorrent

Flow control:

Outstanding packets will never outnumber the window size

Congestion Control:

Implemented Slow Start, Congstion Aviodence, Fast Retransmit and Fast recovery.

Generally, SSTHREASH is half of the maximum window size.

Window size will be set to 1 on data losses, i.e. timeout or 3 duplicate ACKs.

SSTHRESH will be set to max(SSTHRESH/2, 2) when this happens

Discard re-ordered packets

Robustness:

The peer can handle the case of peer crash by setting peer crash timeout to 4 times of the RTO
