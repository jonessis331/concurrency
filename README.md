<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
</head>
<body>

<h1>Concurrency: Channels in C</h1>

<h2>Introduction</h2>
<p>Channels serve as a powerful synchronization model through message passing, analogous to queues with a set maximum size. They facilitate communication between threads with multiple senders and receivers potentially interacting with a given channel concurrently.</p>
<p>Channel constructs are integral in concurrent programming frameworks, significantly in languages such as Go. This library is an implementation of channels in C, offering both blocking and non-blocking send/receive operations. It is designed to enable efficient inter-thread communication and synchronization without relying on less performant mechanisms like fixed-duration sleeping or busy-waiting.</p>

<h2>Features</h2>
<p>This implementation supports:</p>
<ul>
    <li>Creating channels with specified sizes</li>
    <li>Blocking and non-blocking send/receive operations</li>
    <li>Close and destroy operations for channel management</li>
    <li>A selection mechanism for multiple channel operations</li>
</ul>

<h2>Setup</h2>
<p>Please consolidate all files from the 'config', 'src', and 'tests' directories into a unified directory structure to ensure proper integration with the channel library. It is designed to work with standard C libraries and the POSIX thread library, ensuring compatibility across a wide array of C environments.</p>

<h2>Using the Library</h2>
<p>The channel API provides a suite of functions for channel operations, detailed in <code>channel.c</code> and <code>channel.h</code>. Here are the primary functions available:</p>
<ul>
    <li><code>channel_create</code> - Initializes a new channel of a specified size.</li>
    <li><code>channel_send</code> - Sends data over the channel, blocking if necessary.</li>
    <li><code>channel_receive</code> - Receives data from the channel, blocking if necessary.</li>
    <!-- Add other functions as necessary -->
</ul>

<h2>Building the Code</h2>
<p>Compile the library using a standard C compiler with support for the pthread library. A Makefile is provided for convenience:</p>
<pre><code>make all</code></pre>

<h2>Testing</h2>
<p>To ensure reliability and correctness, an extensive test suite accompanies the library:</p>
<pre><code>make test</code></pre>
<p>Run this command to execute all tests, verifying the correct behavior of channel operations under various conditions.</p>

<h2>Performance</h2>
<p>While performance is a consideration, the primary goal of this implementation is correctness and adherence to the channel model. The library is designed to avoid common inefficiencies such as busy loops and unnecessary CPU consumption.</p>

<h2>Contributions</h2>
<p>The core functionality of this channel library is contained within the <code>channel.c</code> and <code>channel.h</code> files, which are my original contributions. The remaining codebase, including utility and auxiliary functions, was provided as part of the academic coursework to facilitate the testing and demonstration of the channel implementation.</p>
<p>While the supplementary code was not authored by me, modifications have been made where necessary to ensure seamless integration with the channel logic, adhering to the assignment's specifications and objectives.</p>


</body>
</html>
