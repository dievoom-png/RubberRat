<?php
$servername = "localhost";
$username = "toor";
$password = "toor";
$dbname = "connections";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT * FROM instances";
$result = $conn->query($sql);

if ($result->num_rows > 0) {
    $data = $result->fetch_all(MYSQLI_ASSOC);
} else {
    $data = [];
}

echo '<!DOCTYPE html>';
echo '<html>';
echo '<head>';
echo '<title>Instances Data</title>';
echo '<style>';
echo 'table { border-collapse: collapse; width: 100%; }';
echo 'th, td { border: 1px solid black; padding: 8px; text-align: left; }';
echo 'th { background-color: #f2f2f2; }';
echo '</style>';
echo '</head>';
echo '<body>';
echo '<h1>Instances Data</h1>';

if (!empty($data)) {
    echo '<table>';
    echo '<tr>';
    foreach (array_keys($data[0]) as $header) {
        echo '<th>' . htmlspecialchars($header) . '</th>';
    }
    echo '</tr>';
    foreach ($data as $row) {
        echo '<tr>';
        $i=0;
        foreach ($row as $cell) {
            if($i==0){
                $attrib = "<a href='index.php?id=".$cell."'>".$cell."</a>";
                
                echo '<td>' .$attrib . '</td>';

            }
            else{
                echo '<td>' . htmlspecialchars($cell) . '</td>';

            }
            $i+= 1;
        }
        $i =0;
        echo '</tr>';
    }
    echo '</table>';
} else {
    echo '<p>No data found.</p>';
}

echo '</body>';
echo '</html>';

$conn->close();
?>


