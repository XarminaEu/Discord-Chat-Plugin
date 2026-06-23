# Discord Webhook Test-Skript fuer PalworldDiscordPlugin
# Verwendung: .\test_discord_webhook.ps1 -WebhookUrl "https://discord.com/api/webhooks/..."
#
# Hinweis: Die Webhook-URL ist sensibel. Teile sie nicht oeffentlich und
# speichere sie nicht unverschluesselt in Repositories.

param(
    [Parameter(Mandatory=$true, HelpMessage="Discord Webhook URL")]
    [string]$WebhookUrl,

    [string]$Message = "Dies ist eine **Testnachricht** vom PalworldDiscordPlugin. Wenn du das hier siehst, funktioniert der Discord-Webhook!"
)

$payload = @{
    username = "PalworldPlugin-Test"
    content = $Message
} | ConvertTo-Json

Write-Host "[*] Sende Testnachricht an Discord-Webhook..." -ForegroundColor Cyan

try {
    Invoke-RestMethod -Uri $WebhookUrl -Method Post -ContentType "application/json" -Body $payload
    Write-Host "[OK] Testnachricht erfolgreich gesendet!" -ForegroundColor Green
} catch {
    Write-Host "[FEHLER] $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "[HINWEIS] Pruefe, ob die URL gueltig und der Bot nicht rate-limited ist." -ForegroundColor Yellow
    exit 1
}
