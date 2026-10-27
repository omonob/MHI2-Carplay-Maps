"""Public evidence sanity checks and the read-only wireless interface example."""
import csv,hashlib,json,re,struct,subprocess,tempfile,zlib
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'docs/development/2026-10-28'

def main():
 summary=json.loads((DATA/'evidence/mirror-performance-summary.json').read_text(encoding='utf-8-sig'))
 assert summary['decoded_capture']['intervals']==8
 assert abs(summary['software_stage']['mean_ms']-52.8)<1e-6
 assert summary['software_stage']['end_to_end_measured'] is False
 assert summary['after_r4']['encoder_fps_remeasured'] is False
 for name,count in [('decoded-capture-intervals.csv',8),('renderer-windows.csv',25),('encoder-feed-before-r4.csv',80)]:
  with (DATA/'evidence'/name).open(encoding='utf-8-sig') as f:assert len(list(csv.DictReader(f)))==count
 for name in ['software-stage-timing.svg','encoder-feed-period.svg']:ET.parse(DATA/'evidence'/name)
 for name,size in [('native-vc-entry.png',(1280,640)),('oem-dropdown-reference.png',(1280,640)),('mirror-aid10.5-fixture.png',(1010,376)),('mirror-aid12.3-fixture.png',(800,480))]:
  raw=(DATA/'images'/name).read_bytes();assert raw[:8]==b'\x89PNG\r\n\x1a\n';assert struct.unpack('>II',raw[16:24])==size
 provenance=json.loads((DATA/'evidence/image-provenance.json').read_text(encoding='utf-8-sig'))
 for item in provenance['images']:
  assert hashlib.sha256((DATA/item['file']).read_bytes()).hexdigest()==item['sha256']
  assert item['unchanged'] is True
 for doc in DATA.glob('*.md'):
  text=doc.read_text(encoding='utf-8-sig')
  for link in re.findall(r'\]\(([^)]+)\)',text):
   if not link.startswith(('https:','http:','#','mailto:')):assert (doc.parent/link).is_file(),link
  display=re.sub(r'`[^`]+`|\]\([^)]+\)','',text)
  assert not re.search(r'(?<![A-Za-z0-9_])79[01](?![A-Za-z0-9_])',display)
 with tempfile.TemporaryDirectory(prefix='cpvc-public-interface-') as tmp:
  p=Path(tmp);binary=p/'wireless-state-reader'
  subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(ROOT/'examples/wireless-state-reader.c'),'-o',str(binary)],check=True)
  def read():return subprocess.run([str(binary),str(p)],capture_output=True,text=True)
  assert 'enabled=0 generation=0 source=0 status=1' in read().stdout
  def state(enabled,gen):
   body=f'format=1\nenabled={enabled}\ngeneration={gen}\n'.encode()
   return body+f'crc32={zlib.crc32(body):x}\n'.encode()
  (p/'wireless-carplay.state').write_bytes(state(1,1));assert 'enabled=1 generation=1 source=1 status=0' in read().stdout
  (p/'wireless-carplay.previous').write_bytes(state(1,1));(p/'wireless-carplay.state').write_bytes(state(0,2));assert 'enabled=0 generation=2 source=1 status=0' in read().stdout
  (p/'wireless-carplay.state').write_bytes(b'bad');assert 'enabled=1 generation=1 source=2 status=0' in read().stdout
  (p/'wireless-carplay.previous').write_bytes(b'bad');bad=read();assert bad.returncode==2 and 'enabled=0' in bad.stdout
  assert not (p/'settings.state').exists() # no other selector or media-module state mutation
 print('PUBLIC_DEVELOPMENT_EVIDENCE_AND_WIRELESS_READER_PASS')
if __name__=='__main__':main()
