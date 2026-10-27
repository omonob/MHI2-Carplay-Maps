[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$RendererCapture,
 [Parameter(Mandatory=$true)][string]$EncoderProbe,
 [Parameter(Mandatory=$true)][string]$OutputDirectory
)
# Export numeric fields only. Never copy complete unit logs or process listings.
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
if(Test-Path -LiteralPath $OutputDirectory){throw 'Use a new output directory'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
$raw=Get-Content -LiteralPath $RendererCapture -Raw
$probe=Get-Content -LiteralPath $EncoderProbe -Raw
$phase=[regex]::Match($raw,'(?s)ACTIVE_PHASE_BEGIN=3\r?\n(.*?)ACTIVE_PHASE_END=3')
if(!$phase.Success){throw 'Expected final bounded capture phase missing'}
$tap=[regex]::Match($phase.Groups[1].Value,'(?s)TAP_BEGIN=3\r?\n(.*?)TAP_END=3')
$renderer=[regex]::Match($phase.Groups[1].Value,'(?s)RENDER_BEGIN=3\r?\n(.*?)RENDER_END=3')
if(!$tap.Success -or !$renderer.Success){throw 'Capture sections missing'}
$progress=@([regex]::Matches($tap.Groups[1].Value,'t_us=(\d+) pid=(\d+) event=progress source=1264x640 frames=(\d+)'))
if($progress.Count -lt 2){throw 'Insufficient capture samples'}
# A rolling log includes older DIO sessions. Restrict analysis to the final
# capture owner; do not export its process identifier.
$lastOwner=$progress[$progress.Count-1].Groups[2].Value
$progress=@($progress|Where-Object {$_.Groups[2].Value -eq $lastOwner})
$capture=@();for($i=1;$i -lt $progress.Count;$i++){
 $dt=([double]$progress[$i].Groups[1].Value-[double]$progress[$i-1].Groups[1].Value)/1000000
 $df=[int]$progress[$i].Groups[3].Value-[int]$progress[$i-1].Groups[3].Value
 if($dt -le 0 -or $df -le 0){continue}
 $capture+=@([ordered]@{interval=$capture.Count+1;frames=$df;seconds=[math]::Round($dt,6);capture_fps=[math]::Round($df/$dt,6)})
}
$render=@();foreach($m in [regex]::Matches($renderer.Groups[1].Value,'carplay-mirror: render=(\d+) latest_sequence=(\d+) source=1264x640 sender=\d+ latency_ms=(\d+) avg_ms=(\d+) min_ms=(\d+) max_ms=(\d+) samples=(\d+) swap_ms=(\d+) swap_avg_ms=(\d+) swap_min_ms=(\d+) swap_max_ms=(\d+) swap_samples=(\d+)')){
 $render+=@([ordered]@{window=$render.Count+1;render_count=[int]$m.Groups[1].Value;stage_last_ms=[int]$m.Groups[3].Value;stage_avg_ms=[int]$m.Groups[4].Value;stage_min_ms=[int]$m.Groups[5].Value;stage_max_ms=[int]$m.Groups[6].Value;samples=[int]$m.Groups[7].Value;swap_avg_ms=[int]$m.Groups[9].Value;swap_max_ms=[int]$m.Groups[11].Value})
}
# The historical diagnosis used the final25 reporting windows, not warm-up.
if($render.Count -lt 25){throw 'Expected25 final renderer windows'}
$render=@($render|Select-Object -Last 25)
for($i=0;$i -lt $render.Count;$i++){$render[$i].window=$i+1}
$feed=@();$matches=@([regex]::Matches($probe,'FEED t_us=(\d+) rate=(\d+) encoder_ms=(\d+) interval_ms=(\d+)'))
if($matches.Count -lt 2){throw 'Encoder feed samples missing'}
$start=[long]$matches[0].Groups[1].Value
foreach($m in $matches){$feed+=@([ordered]@{event=$feed.Count+1;relative_us=([long]$m.Groups[1].Value-$start);requested_rate=[int]$m.Groups[2].Value;encoder_interval_ms=[int]$m.Groups[4].Value})}
$end=[regex]::Match($probe,'OUTPUT_RATE_PROBE_END feed_events=(\d+) elapsed_ms=(\d+) measured_feed_fps=([0-9.]+) last_update_rate=(\d+)')
if(!$end.Success -or [int]$end.Groups[1].Value -ne $feed.Count){throw 'Probe receipt does not match samples'}
$capture | ForEach-Object {[pscustomobject]$_}|Export-Csv -LiteralPath (Join-Path $OutputDirectory 'decoded-capture-intervals.csv') -NoTypeInformation -Encoding UTF8
$render | ForEach-Object {[pscustomobject]$_}|Export-Csv -LiteralPath (Join-Path $OutputDirectory 'renderer-windows.csv') -NoTypeInformation -Encoding UTF8
$feed | ForEach-Object {[pscustomobject]$_}|Export-Csv -LiteralPath (Join-Path $OutputDirectory 'encoder-feed-before-r4.csv') -NoTypeInformation -Encoding UTF8
$c=@($capture|ForEach-Object {$_.capture_fps});$r=@($render|ForEach-Object {$_.render_count});$lat=@($render|ForEach-Object {$_.stage_avg_ms})
$summary=[ordered]@{
 schema=1;platform='MU1102 AID 10.5';test_date='2026-10-27';phase='R3 before mirror-only rate-request patch'
 decoded_capture=@{intervals=$capture.Count;min_fps=($c|Measure-Object -Minimum).Minimum;max_fps=($c|Measure-Object -Maximum).Maximum}
 renderer=@{windows=$render.Count;window_duration='approximately 1 second, not exact FPS intervals';min_frames=($r|Measure-Object -Minimum).Minimum;max_frames=($r|Measure-Object -Maximum).Maximum;mean_frames=[math]::Round(($r|Measure-Object -Average).Average,3)}
 software_stage=@{definition='decoded surface queued timestamp to completion of EGL swap; excludes phone input, native encoder and cluster display';statistic='arithmetic mean of 25 per-window avg_ms values';mean_ms=[math]::Round(($lat|Measure-Object -Average).Average,3);window_mean_min_ms=($lat|Measure-Object -Minimum).Minimum;window_mean_max_ms=($lat|Measure-Object -Maximum).Maximum;end_to_end_measured=$false}
 encoder_before_patch=@{feed_events=$feed.Count;elapsed_ms=[int]$end.Groups[2].Value;measured_feed_fps=[double]::Parse($end.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture);requested_rate=[int]$end.Groups[4].Value;physical_panel_fps_measured=$false}
 after_r4=@{requested_fps=30;applies_to='AID 10.5 CP mirroring only';user_hot_enable_disable='confirmed working';encoder_fps_remeasured=$false;end_to_end_latency_remeasured=$false}
 provenance=@{renderer_log_sha256=(Get-FileHash -LiteralPath $RendererCapture).Hash.ToLowerInvariant();encoder_log_sha256=(Get-FileHash -LiteralPath $EncoderProbe).Hash.ToLowerInvariant();export='numeric allowlist, normalized encoder time, no complete raw logs'}
}
$summary|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $OutputDirectory 'mirror-performance-summary.json') -Encoding UTF8
function Plot([string]$Title,[string]$Caption,[object[]]$Values,[double]$Min,[double]$Max,[string]$Unit,[string]$Name){
 $svg=New-Object Text.StringBuilder
 [void]$svg.AppendLine('<svg xmlns="http://www.w3.org/2000/svg" width="900" height="420" viewBox="0 0 900 420"><rect width="900" height="420" fill="#fff"/><g font-family="sans-serif" fill="#172331">')
 [void]$svg.AppendLine('<text x="46" y="40" font-size="24">'+$Title+'</text><text x="46" y="68" font-size="13">'+$Caption+'</text>')
 for($i=0;$i -le 4;$i++){
  $v=$Min+($Max-$Min)*$i/4;$y=330-$i*55;$label=$v.ToString('0.##',[Globalization.CultureInfo]::InvariantCulture)
  [void]$svg.AppendLine('<path d="M80 '+$y+' H850" stroke="#dbe3ea"/><text x="34" y="'+($y+5)+'" font-size="14">'+$label+'</text>')
 }
 $points=@();for($i=0;$i -lt $Values.Count;$i++){$x=80+770*$i/($Values.Count-1);$y=330-220*([double]$Values[$i]-$Min)/($Max-$Min);$points+=($x.ToString('0.##',[Globalization.CultureInfo]::InvariantCulture)+','+$y.ToString('0.##',[Globalization.CultureInfo]::InvariantCulture))}
 [void]$svg.AppendLine('<polyline points="'+($points -join ' ')+'" fill="none" stroke="#087fa0" stroke-width="3"/>')
 [void]$svg.AppendLine('<text x="80" y="101" font-size="14">'+$Unit+'</text><text x="80" y="356" font-size="14">1</text><text x="820" y="356" font-size="14">'+$Values.Count+'</text><text x="345" y="382" font-size="14">Reporting window / sample index</text></g></svg>')
 [IO.File]::WriteAllText((Join-Path $OutputDirectory $Name),$svg.ToString(),[Text.UTF8Encoding]::new($false))
}
Plot 'Software-stage timing before R4' 'Window averages only; NOT phone-to-instrument end-to-end latency.' $lat 40 65 'avg_ms (ms)' 'software-stage-timing.svg'
$intervals=@($feed|ForEach-Object {$_.encoder_interval_ms})
Plot 'Native encoder feed before R4' '80 feed events; 100 ms period; measured 10.000 fps. Post-patch 30 fps has not been remeasured.' $intervals 97 103 'Encoder interval (ms)' 'encoder-feed-period.svg'
Write-Output ($summary|ConvertTo-Json -Depth 5)
