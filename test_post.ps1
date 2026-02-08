$body = @{
    name = "Test Park"
    type = "park"
    x = 12.8618
    y = 77.4936
} | ConvertTo-Json

Invoke-WebRequest -Uri "http://localhost:8080/api/venue" -Method POST -ContentType "application/json" -Body $body
