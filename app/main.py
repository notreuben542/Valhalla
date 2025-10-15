from fastapi import FastAPI
from fastapi.responses import JSONResponse
from app.routes import order_routes


app = FastAPI(title="Crypto Matching Engine", version="1.0.0")

app.include_router(order_routes.router, prefix="/api/v1",tags=["orders"])



@app.get("/")
def root():
    return JSONResponse(content={"message": "Crypto Matching Engine is running."})

def main():
    import uvicorn
    uvicorn.run("app.main:app", host="0.0.0.0", port=8000, reload=True)
    print("Hello from crypto-matching-engine!")


if __name__ == "__main__":
    main()
