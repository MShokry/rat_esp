<?
$data = array(
	'ssid' => 'Ebni.Eitesal',
	'ssid_pwd' => 'eitesalngo',
	'update_server'=>'7elol.com',
	'update_server_page'=>'/update/config.php',
	'server'=>'7elol.com',
	'server_page'=>'/update/rfid.php',
	'place'=>'Nest12',
	'ip'=>'192.168.1.59',
	'gateway'=>'192.168.1.1',
	'subnet'=>'255.255.255.0');
header("Content-Type: application/json; charset=UTF-8");
echo json_encode( $data );
?>