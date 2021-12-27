<p align="center">
  <a href="https://rclone.org/">
    <img width="20%" alt="Reactor" src="https://user-images.githubusercontent.com/4536448/147360224-61e03a6e-4252-4e03-afad-ca09b8f73996.png">
  </a>
</p>

# ClipNet
A LAN-level clipboard utility.

## Summary
Do you use multiple machines during the day?  Are they all on the same LAN segment?  Are you having to use third-party sites (like cl1p.net) to exchange text and URLs between them?  Well, then, you need `ClipNet`!

## There Has To Be An Easier Way®
I fall into the above category.  I use three or more machines during the day for various things--one is for communication software, like Skype and Slack, one is for software development, etc.  Exchanging text information between them requires me to go out to the cloud to paste it, and then out to that same cloud again to retreive it.  That alone is a pain in the ass, but when I think about the security nightmare involved when trying to convey sensitive information between machines that are literally feet apart, I just knew there had to be an easier way.  Thus, the idea for `ClipNet` was born.

`ClipNet` uses UDP to create a "private" multicasting group where clipboard data from one machine is instantly transmitted to all other members of the group.  These other members react to the event by placing the event data immediately onto their respective clipboards.  In this fashion, the clipboards of all machines are instantly syncrhonized, and data is transmitted without any manual intervention--and, more importantly, any third-party involvement.

## Security
Exchanging plain-text data on your "isolated" home network may not send *you* into a sweaty, paranoid panick ... but *you* are not *me*.  I have integrated two options for data security into `ClipNet` to protect the clipboard data as it travels along the twisted pair.

### SimpleCrypt
The first is [SimpleCrypt](https://wiki.qt.io/Simple_encryption_with_SimpleCrypt).  This is more of an obfuscating library, and not really a full-fledged cryptographic system.  It provides some light-weight obscuring of data, and in most cases, provides sufficient coverage of your clipboard plain text.  This is the default security type in the build, as the code for `SimpleCrypt` is included directly into the repo assets.

### Crypto++
You also have the option of using [Crypto++](https://github.com/weidai11/cryptopp) for implementing actual, heavy weight cryptographic encryption of your clipboard data.  Since this is an external library, it is not the default solution for the `ClipNet` build.

> **_NOTE:_** The Windows binary distribution available for this project uses `Crypto++` instead of `SimpleCrypt`.  It employs CFB mode encryption (AES + SHA256) for encrypting clipboard text.

## Options
`ClipNet` runs in the task tray.  Initially, you will need to configure it, and will likely also need to allow it through your firewall (Windows will automatically prompt you to allow the process).

### Multicast groups
You can enable IPv4 multicast, IPv6 multicast, or both, depending upon your network configuration and communication needs.

![ClipNet_2021-12-24_07-40-04](https://user-images.githubusercontent.com/4536448/147360182-6a44dbcd-a440-402d-ae12-25a2ac2360e9.png)

Advanced users will know what they need--most everybody else will only need to use IPv4 multicasting, as IPv6 is not widely used in home networks (nor even in corporate offices, for that matter).

When you enable one of the protocols, you can use the default address values (not recommended), or you can enter multicast addresses manually (if you know what you're doing), or you can tell `ClipNet` to randomize the multicast IP addresses for you.

![2021-12-24_07-42-55](https://user-images.githubusercontent.com/4536448/147360187-ede36f1e-73e4-48c6-a35e-5e2e53b37b97.gif)

In the latter two cases, you need to be sure that the resulting IP address is used by all other machines you expect to be members of your `ClipNet` multicast group.

After establishing your multicast address(es), you can then press the "Join" button to launch `ClipNet` into the specified multicast group, and clipboard activity will begin flowing between members.  However, you may want to peform some further configuration before doing so.

### Automatically rejoining
Enabling this option will cause `ClipNet` to rejoin the previous multicast group whenever it starts.

![ClipNet_2021-12-24_07-40-55](https://user-images.githubusercontent.com/4536448/147360184-ebdf9104-5e91-4d55-b8f8-bb54a601b98b.png)

This is handy for use with the next option.

### Automatic startup
You can tell `ClipNet` to start automatically when you log in.

![ClipNet_2021-12-24_07-42-03](https://user-images.githubusercontent.com/4536448/147360185-6280d6a7-a088-42fe-aff6-30bfe3de2e13.png)

Note that this option will only be avilable on Windows system--other operating systems have external mechanisms for achieving this.

Enabling automatic rejoin along with this option will result in a fire-and-forget run of `ClipNet`, and it will always be available when you log into your machine.

### Secure operation
The last section is concerned with securing your interactions on the network.

#### Encryption
Encryption of network payloads is not enabled by default--I recommend you enable it.  When you do, you should enter a passphrase that will be used to secure your clipboard data.

![2021-12-24_07-44-47](https://user-images.githubusercontent.com/4536448/147360195-4c779f33-c539-4353-9112-bd6399f7698f.gif)

This passphrase needs to be identical on each member of the multicast group in order for the encrypted payload from one member to be successfully decrypted on all others.

#### Clearing the clipboard
Enabling this option tells `ClipNet` to clear the text contents of the local machine clipboard after a given timeout period following its placement.  This is handy if you routinely exchange very sensitive data that you don't want lingering in plain text on the system clipboard.

## Notes
* `ClipNet` only processes text MIME types on the clipboard.  No other clipboard data types are currently supported.
* `Auto-launch` is a work in progress and does not currently function.
* Passphrases are currently stored in clear text in the configuration file.
* Icons/images for the project were derived from [glyphs.fyi](https://glyphs.fyi/dir?i=handHoldingSeedling&v=poly&w).
* `ClipNet` was developed using Qt 5.14.1.

## Happy clipping
I hope you find `ClipNet` useful.