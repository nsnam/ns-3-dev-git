.

<h1><i>#9 :</i>Implementation of LEDBAT++ in ns-3.</h1>
<h3>Planning & Design Description Doc can be found here : <a href = "https://github.com/Awanit512/3-TCP-LEDBAT_in_WiFi/blob/exhaustive_evaluation_ledbat/scratch/TCP-Ledbat-Evaluation/planning_and%20_design_readme.md"> planning_and_design_readme.md </a> </h3>

<table>
<tr>
  <td><b>Brief:</b></td>
  <td>
Low Extra Delay Background Transport (LEDBAT) is a popular Less than Best Effort
(LBE) TCP which Apple uses for its iOS updates and by BitTorrent applications. Recently,
there have been a few enhancements to LEDBAT, and one of the resulting algorithms has
been named LEDBAT++. It is a part of the Windows operating system. This project aims to
implement a model of LEDBAT++ in ns-3. LEDBAT is already implemented in ns-3 and is
available in the mainline.
  </td>
</tr>
  <tr>
  <td><b>Contributors:</b></td>
  <td>
 
  
   Awanit Ranjan      [181CO161] (<a href="https://github.com/Awanit512"> Github Repo </a>)     <br />
   

  </td>
</tr>

<tr>
 
 <td><b>Recommended Reading</b>:</td>
 <td> 
  <table>
   <thead>
    <tr>
     <td><b>Description</b></td>
      <td><b>Link</b></td>
   </tr>
  </thead>
  <tbody>
    
   <tr>
     <td>LEDBAT: RFC 6817</td>
     <td> <a href="https://tools.ietf.org/html/rfc6817"> Link </a></td>
   </tr>

   <tr>
     <td>LTCP examples of ns-3</td>
     <td> <a href=""> ns-3.xx/examples/tcp/ </a></td> 
    </tr>


   <tr>
     <td>Exploration and evaluation of traditional TCP congestion control techniques</td>
     <td> <a href=""> ns-3.xx/src/internet/model/tcp-ledbat{.h, .cc} </a></td> 
    </tr>
    
    
   <tr>
     <td> LEDBAT++: Internet Draft </td>
     <td> <a href="https://tools.ietf.org/html/draft-irtf-iccrg-ledbat-plus-plus-01"> Link </a> </td> 
    </tr>
    

   
  </tbody>
  </table>
</td>
    </tr>
   
   
   <tr>
 
 <td><b>LEDBAT Operations/Algorithm</b>:</td>
 <td> 
  <table>
   <thead>
    <tr>
      <td><b>Sender side and Receiver side operations</b></td>
      <td><b>Pseudocode / Mechanisms</b></td>
   </tr>
  </thead>
  <tbody>
   <tr>
     <td>Receiver side </td>
     <td><pre lang="csharp">
   on data_packet:
       remote_timestamp = data_packet.timestamp
       acknowledgement.delay = local_timestamp() - remote_timestamp
       // fill in other fields of acknowledgement
       acknowledgement.send()
   </pre></td>
   </tr>

   <tr>
     <td>Sender side </td>
     <td><pre lang="csharp">
on initialization:
  set all NOISE_FILTER delays used by current_delay() to +infinity
  set all BASE_HISTORY delays used by base_delay() to +infinity
  last_rollover = -infinity # More than a minute in the past.
</pre>
<pre lang="csharp">
on acknowledgement:
  delay = acknowledgement.delay
  update_base_delay(delay)
  update_current_delay(delay)
  queuing_delay = current_delay() - base_delay()
  off_target = TARGET - queuing_delay + random_input()
  cwnd += GAIN * off_target / cwnd
  // flight_size() is the amount of currently not acked data.
  max_allowed_cwnd = ALLOWED_INCREASE + TETHER*flight_size()
  cwnd = min(cwnd, max_allowed_cwnd)
</pre>
<pre lang="csharp">
random_input()
  // random() is a PRNG between 0.0 and 1.0
  // NB: RANDOMNESS_AMOUNT is normally 0
  RANDOMNESS_AMOUNT * TARGET * ((random() - 0.5)*2)
</pre>
<pre lang="csharp">
update_current_delay(delay)
  // Maintain a list of NOISE_FILTER last delays observed.
  forget the earliest of NOISE_FILTER current_delays
  add delay to the end of current_delays
</pre>
<pre lang="csharp">
current_delay()
  min(the NOISE_FILTER delays stored by update_current_delay)
</pre>
<pre lang="csharp">
update_base_delay(delay)
  // Maintain BASE_HISTORY min delays. Each represents a minute.
  if round_to_minute(now) != round_to_minute(last_rollover)
    last_rollover = now
    forget the earliest of base delays
    add delay to the end of base_delays
  else
    last of base_delays = min(last of base_delays, delay)
</pre>
 <pre lang="csharp">
base_delay()
  min(the BASE_HISTORY min delays stored by update_base_delay)
   </pre></td>
    </tr>
  </tbody>
  </table>
</td>
    </tr>
   
  
</table>

   
  


   
  
