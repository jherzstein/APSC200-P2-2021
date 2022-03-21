package main

import (
	"encoding/json"
	"fmt"
	"html/template"
	"net/http"
	"os"
	//"net/url"
	"strconv"
	"time"
)

type DataPoint struct {
	Time time.Time
	Time_Arduino float64
}

const fn = "/home/jordanh/motiondata.json"
//const fn = "/tmp/data.json"
var tmpl = template.Must(template.New("").Parse(`
<!DOCTYPE html>
<html>
<head>
	<title>Racoon Deterrant Database</title>
</head>
<body>
	{{range .}}
	<div><b>Time Server Received{{.Time.Format "Monday, 3:04:05 PM"}}: </b> Arduino Time (seconds): {{.Time_Arduino}}<br></div>
	{{end}}
</body>
</html>
`))

func main() {
	http.HandleFunc("/test/post.php", func(w http.ResponseWriter, r *http.Request) {
		fmt.Println(r.URL)
		//temp, err := r.URL.Query().Get("temp"), 64
		//temp, err := url.Parse(r.URL.Query().Get("temp"))
		temp, err := strconv.ParseFloat(r.URL.Query().Get("temp"), 64)
		if err != nil {
			panic(err)
		}

		data, err := readData()
		if err != nil {
			fmt.Println("Warning:", err)
		}
		data = append(data, DataPoint{Time: time.Now(), Time_Arduino: temp})

		if err := writeData(data); err != nil {
			panic(err)
		}
	})
	http.HandleFunc("/test/time.html", func(w http.ResponseWriter, r *http.Request) {
		data, err := readData()
		if err != nil {
			fmt.Println("Warning:", err)
		}
		if err := tmpl.Execute(w, data); err != nil {
			panic(err)
		}
	})
	panic(http.ListenAndServe(":80", http.DefaultServeMux))
}

func writeData(d []DataPoint) error {
	f, err := os.OpenFile(fn, os.O_WRONLY|os.O_TRUNC|os.O_CREATE, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

	return json.NewEncoder(f).Encode(d)
}
func readData() ([]DataPoint, error) {
	f, err := os.OpenFile(fn, os.O_RDONLY, 0644)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var d []DataPoint
	if err := json.NewDecoder(f).Decode(&d); err != nil {
		return nil, err
	}
	return d, nil
}
